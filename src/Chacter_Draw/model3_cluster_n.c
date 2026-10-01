/*
 * Model3 cluster (morph target) animation (Chacter_Draw). VU0 adds each weighted cluster's
 * vertex deltas to the model's default cluster nodes, working in scratchpad memory; the
 * results are converted to 12.4 fixed point for the VU1 renderer.
 */
#include "sh2.h"
#include "model3_helpers.h"
#include "sdk/eekernel.h"
#include "sdk/libdma.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

extern u_long128 model3_mpg0_cluster_load[];

static void LoadProgram(void) {
    static u_long128 packet_buffer[4] __attribute__((aligned(32)));
    static int initialized;
    sceVif0Packet packet;
    sceVif0Packet *pk;

    if (!initialized) {
        pk = &packet;
        sceVif0PkInit(pk, (u_long128 *)(((unsigned int)packet_buffer & 0x0FFFFFFF) | 0x20000000));
        sceVif0PkCall(pk, model3_mpg0_cluster_load, 0);
        sceVif0PkEnd(pk, 0);
        sceVif0PkTerminate(pk);
        initialized = 1;
    }
    ktVif0Send(packet_buffer, 1);
}

static void MakeTransferDefaultClusterNodesPacket(u_long128 *packet_buffer, struct DefaultClusterNode *nodes_top, int n_nodes) {
    sceVif0Packet packet;
    sceVif0Packet *pk;
    int qwc;

    qwc = (n_nodes * sizeof(struct DefaultClusterNode) + 15) / 16;
    pk = &packet;
    sceVif0PkInit(pk, (u_long128 *)(((unsigned int)packet_buffer & 0x0FFFFFFF) | 0x20000000));
    sceVif0PkRef(pk, (u_long128 *)nodes_top, qwc, (n_nodes - 1) | 0x04000000, ((n_nodes & 0xFF) << 16) | 0x69000000, 0);
    sceVif0PkEnd(pk, 0);
    sceVif0PkAddCode(pk, 0x14000000);
    sceVif0PkAddCode(pk, 0x10000000);
    sceVif0PkTerminate(pk);
}

static void TransferDefaultClusterNodes(struct Model *model) {
    int n_cluster_nodes;
    float (*cluster_nodes)[4];
    struct DefaultClusterNode *default_cluster_nodes;
    int i;
    int n_rest_nodes;
    int n_nodes;
    int sadr;
    u_long128 *packet_buffer;
    sceDmaChan *toSPR;
    sceDmaChan *fromSPR;

    n_cluster_nodes = model->n_cluster_nodes;
    cluster_nodes = model3_junk.cluster_nodes;
    default_cluster_nodes = (struct DefaultClusterNode *)((char *)model + model->cluster_nodes_offset);
    for (i = 0; i < n_cluster_nodes; i += n_nodes) {
        n_rest_nodes = n_cluster_nodes - i;
        n_nodes = n_rest_nodes < 256 ? n_rest_nodes : 256;
        sadr = (i < 0x300 ? i : 0x300) * 16;
        packet_buffer = ktVif0PkBufNext();
        MakeTransferDefaultClusterNodesPacket(packet_buffer, &default_cluster_nodes[i], n_nodes);
        ktVif0Send(packet_buffer, 1);
        ktVif0Wait2();
        fromSPR = sceDmaGetChan(9);
        fromSPR->sadr = (void *)sadr;
        sceDmaSendN(fromSPR, (void *)0x11004000, n_nodes);
        while (sceDmaSync(fromSPR, 0, 0)) {
        }
        if (i >= 0x300) {
            toSPR = sceDmaGetChan(8);
            toSPR->sadr = (void *)sadr;
            sceDmaSendN(toSPR, cluster_nodes[i], n_nodes);
            while (sceDmaSync(toSPR, 0, 0)) {
            }
        }
    }
}

static void MakeApplyClusterPacket(u_long128 *packet_buffer, struct ClusterElement *top, int n, float weight) {
    sceVif0Packet packet;
    sceVif0Packet *pk;
    int qwc;

    qwc = (n * sizeof(struct ClusterElement) + 15) / 16;
    pk = &packet;
    sceVif0PkInit(pk, (u_long128 *)(((unsigned int)packet_buffer & 0x0FFFFFFF) | 0x20000000));
    sceVif0PkRef(pk, (u_long128 *)top, qwc, (n - 1) | 0x04000000, (n << 16) | 0x6D000000, 0);
    sceVif0PkEnd(pk, 0);
    sceVif0PkAddCode(pk, n | 0x60010000);
    PkAddFloat(pk, weight);
    sceVif0PkAddCode(pk, 0x14000002);
    sceVif0PkAddCode(pk, 0x10000000);
    sceVif0PkTerminate(pk);
}

static void ApplyCluster(struct Model *model, struct Cluster *cluster, float weight) {
    float (*cluster_nodes)[4];
    int n_nodes;
    struct ClusterElement *elements;
    int i;
    int n_rest_nodes;
    int n;
    int itop;
    u_long128 *packet_buffer;
    sceDmaChan *toSPR;
    float (*spr)[4];
    union Q *top;
    union Q *end;
    union Q *p;
    int index;
    float *v;

    cluster_nodes = model3_junk.cluster_nodes;
    n_nodes = cluster->n_nodes;
    elements = (struct ClusterElement *)((char *)model + cluster->element_offset);
    itop = 0x300;
    i = 0;
    while (i < n_nodes) {
        n_rest_nodes = n_nodes - i;
        n = n_rest_nodes < 240 ? n_rest_nodes : 240;
        packet_buffer = ktVif0PkBufNext();
        MakeApplyClusterPacket(packet_buffer, &elements[i], n, weight);
        ktVif0Send(packet_buffer, 1);
        ktVif0Wait2();
        toSPR = sceDmaGetChan(9);
        toSPR->sadr = (void *)(itop * 16);
        sceDmaSendN(toSPR, (void *)0x11004000, n);
        while (sceDmaSync(toSPR, 0, 0)) {
        }
        spr = (float (*)[4])0x70000000;
        top = (union Q *)spr[itop];
        end = (union Q *)((unsigned int)top + n * sizeof(union Q));
        for (p = top; p < end; p++) {
            index = p->iv[3];
            if (index < itop) {
                v = spr[index];
            } else {
                v = cluster_nodes[index];
            }
            sceVu0AddVector(v, v, p->fv);
        }
        i += n;
    }
}

/**
 * Applies a model's cluster weights: starts from the default cluster nodes, adds every
 * cluster with a non-zero weight, and leaves the nodes in model_common_work as 12.4 fixed point.
 * @param model the struct Model
 * @param work  its work (cluster_weights)
 */
void Model3UpdateClusters(struct Model *model, struct ModelWork *work) {
    int n_cluster_nodes;
    float (*cluster_nodes)[4];
    struct Cluster *clusters;
    int n_clusters;
    int i;
    float weight;
    struct Cluster *cluster;

    n_cluster_nodes = model->n_cluster_nodes;
    if (n_cluster_nodes != 0) {
        LoadProgram();
        model3_junk.cluster_nodes = model_common_work->cluster_nodes;
        cluster_nodes = model3_junk.cluster_nodes;
        TransferDefaultClusterNodes(model);
        clusters = (struct Cluster *)((char *)model + model->clusters_offset);
        n_clusters = model->n_clusters;
        for (i = 0; i < n_clusters; i++) {
            weight = work->cluster_weights[i];
            if (weight) {
                cluster = &clusters[i];
                ApplyCluster(model, cluster, weight);
            }
        }
        {
            sceDmaChan *fromSPR;
            float (*spr)[4];
            int n;
            int i;

            fromSPR = sceDmaGetChan(8);
            n = n_cluster_nodes < 0x300 ? n_cluster_nodes : 0x300;
            spr = (float (*)[4])0x70000000;
            for (i = 0; i < n; i++) {
                void *p = spr[i];
                ktVu0FTOI4VectorXYZ(p, p);
            }
            fromSPR->sadr = 0;
            sceDmaSendN(fromSPR, cluster_nodes, n);
            for (; i < n_cluster_nodes; i++) {
                void *p = cluster_nodes[i];
                ktVu0FTOI4VectorXYZ(p, p);
            }
            while (sceDmaSync(fromSPR, 0, 0)) {
            }
        }
        SyncDCache(cluster_nodes, cluster_nodes + n_cluster_nodes);
    }
}
