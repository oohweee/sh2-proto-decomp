/*
 * Character cluster (morph target) animation (Chacter_Draw): per character, a set of cluster
 * weights evaluated each frame from keyframed weight data, following the character's
 * skeletal animation clock.
 */
#include "sh2.h"
#include "libc/stdlib.h"
#include "libc/string.h"

struct ClusterAnimeWork sh2cluster;
float cluster_weight_data[32][20];

static float Calc1(void *cluster, int frame, int n_frames, short f_counter);

static float (*calc_func_list[])(void *, int, int, short) = { NULL, Calc1 };

static unsigned int DataId(void *data) {
    struct Header *header;

    header = data;
    return header->id;
}

static unsigned int NClusters(void *data) {
    struct Header *header;

    header = data;
    return header->n_clusters;
}

static unsigned int NFrames(void *data) {
    struct Header *header;

    header = data;
    return header->n_frames;
}

static void *NthCluster(void *data, int n) {
    struct Header *header;

    header = data;
    return (char *)data + header->offsets[n];
}

static float CalcDummy(void) {
    return 0.0f;
}

static float Calc1(void *cluster, int frame, int n_frames, short f_counter) {
    struct Cluster1 *cp = cluster;
    int n_keys = cp->n_keys;
    struct Element1 *ep = cp->elements;
    int i0 = 0;
    int i1 = n_keys - 1;
    int mid = (i0 + i1) >> 1;
    int mid2;
    float t_pre;
    float t;

    if (n_keys <= 0) {
        return 0.0f;
    }
    if (n_keys < 2) {
        return (1.0f / 4096.0f) * ep[0].weight;
    }
    if (frame == n_frames - 1) {
        mid = i1;
        mid2 = 0;
        t = 0.0f;
        t_pre = 1.0f;
        t += t_pre * ((1.0f / 4096.0f) * f_counter);
    } else {
        while (frame < ep[mid].frame || ep[mid + 1].frame < frame) {
            if (frame < ep[mid].frame) {
                i1 = mid;
            } else {
                i0 = mid;
            }
            mid = (i0 + i1) >> 1;
        }
        mid2 = mid + 1;
        if (frame - ep[mid].frame == ep[mid2].frame - ep[mid].frame) {
            mid = mid2;
            mid2++;
        }
        t = (float)(frame - ep[mid].frame) / (float)(ep[mid2].frame - ep[mid].frame);
        t_pre = 1.0f / abs(ep[mid2].frame - ep[mid].frame);
        t += t_pre * ((1.0f / 4096.0f) * f_counter);
    }
    return (1.0f / 4096.0f) * ((1.0f - t) * ep[mid].weight + t * ep[mid2].weight);
}

/** Clears every cluster animation slot. */
void shCharacterInitCluster(void) {
    memset(&sh2cluster, 0, sizeof(sh2cluster));
}

/**
 * Claims cluster animation slot index.
 * @param n_clusters number of clusters (weights) of the model
 * @param index      slot, which also selects the weight buffer
 * @return the slot
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 364
struct shClusterAnime *ClusterAnimeNew(int n_clusters, int index) {
    struct shClusterAnime *cap;

    cap = &sh2cluster.ca[index];
    memset(cap, 0, sizeof(*cap));
    cap->used = 1;
    cap->n_clusters = n_clusters;
    if (n_clusters) {
        cap->weights = cluster_weight_data[index];
        assert_dw(cap->weights);
    }
    return cap;
}

/** Releases a cluster animation: clears weight buffer index if cap is in use (cap itself stays marked used). */
void ClusterAnimeDelete(struct shClusterAnime *cap, int index) {
    if (cap && cap->used) {
        memset(cluster_weight_data[index], 0, sizeof(cluster_weight_data[index]));
    }
}

static void ClearWeights(struct shClusterAnime *cap) {
    int n_clusters;
    float *weights;
    int i;

    if (cap == NULL) {
        return;
    }
    n_clusters = cap->n_clusters;
    weights = cap->weights;
    for (i = 0; i < n_clusters; i++) {
        weights[i] = 0;
    }
}

/**
 * Starts cluster animation data on cap from frame 0; NULL stops it and zeroes the weights.
 * Data with a bad id or another cluster count is refused with a message.
 */
void ClusterAnimeSet(struct shClusterAnime *cap, void *data) {
    int n_clusters;
    int data_id;

    if (cap == NULL) {
        return;
    }
    if (data == NULL) {
        cap->data = NULL;
        ClearWeights(cap);
        return;
    }
    data_id = DataId(data);
    if (data_id != 0x29843918 && data_id != 0x29853918) {
        printf("ClusterAnimeSet: illegal data\n");
        return;
    }
    n_clusters = NClusters(data);
    if (n_clusters != cap->n_clusters) {
        printf("ClusterAnimeSet: n_clusters mismatch\n");
        return;
    }
    cap->data = data;
    cap->frame_no = 0;
    cap->frame_updated = 1;
}

/**
 * Evaluates cap's weights for the current frame of the skeletal animation ap (looping or
 * clamping at the end like it).
 */
void ClusterAnimeExec(struct shClusterAnime *cap, struct shAnime3d *ap) {
    struct Header *header;
    int n_frames;
    int n_clusters;
    int frame;
    int revision;
    int total_counter;
    short frame_counter;
    float (*calc_func)(void *, int, int, short) = NULL;
    int i;
    void *cluster;

    if (cap == NULL) {
        return;
    }
    if (!cap->used) {
        return;
    }
    if (cap->data == NULL) {
        return;
    }
    header = cap->data;
    n_frames = NFrames(header);
    if (cap->frame_updated) {
        cap->frame_updated = 0;
        frame_counter = 0;
    } else {
        total_counter = ap->total_count;
        frame_counter = total_counter & 0xFFF;
        cap->frame_no = total_counter >> 12;
    }
    if (ap->anim_a->loop) {
        if (cap->frame_no >= n_frames) {
            cap->frame_no -= n_frames;
        }
    } else if (cap->frame_no >= n_frames - 1) {
        cap->frame_no = n_frames - 1;
        frame_counter = 0;
    }
    if (cap->frame_no < 0) {
        cap->frame_no = 0;
        frame_counter = 0;
    }
    frame = cap->frame_no;
    revision = header->revision;
    if (revision < ARRAY_COUNT(calc_func_list)) {
        calc_func = calc_func_list[revision];
    }
    if (calc_func == NULL) {
        calc_func = (float (*)(void *, int, int, short))CalcDummy;
    }
    n_clusters = NClusters(header);
    for (i = 0; i < n_clusters; i++) {
        cluster = NthCluster(cap->data, i);
        cap->weights[i] = calc_func(cluster, frame, n_frames, frame_counter);
    }
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 625
/*
 * Matching: stand-in for a function the original linker dead-stripped
 * (config/stripped_functions.txt; tools/mwcc_fixup.py empties it). Its "cap" and
 * "cap->used" literals, pooled with ClusterAnimeGetWeights', stay in .rodata ahead of
 * that function's assert formats. Name unknown; the body is a guess (a cluster-count accessor).
 */
int __stripped_clani_code(struct shClusterAnime *cap) {
    assert(cap);
    assert(cap->used);
    return cap->n_clusters;
}

/**
 * Copies cap's current weights to weights.
 * @return weights
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 644
float *ClusterAnimeGetWeights(struct shClusterAnime *cap, float *weights) {
    int n_clusters;
    int i;

    assert(cap);
    assert(cap->used);
    n_clusters = cap->n_clusters;
    for (i = 0; i < n_clusters; i++) {
        weights[i] = cap->weights[i];
    }
    return weights;
}
