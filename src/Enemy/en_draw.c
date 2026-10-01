/*
 * en_draw.c: the enemy radar, a top-down overlay (playing.radar) that draws the enemies, the
 * collision walls and columns around the player as GS primitives.
 */
#include "enemy.h"

/* pack the x/y/z words of an int vector into a GS XYZ */
static inline unsigned long _shPackXYZ(int *v) {
    unsigned long r;

    __asm__ __volatile__("
    lq     %0, 0x0(%1)
    pexch  t7, %0
    pextuw %0, zero, %0
    pextlw %0, %0, t7
    " : "=r"(r) : "r"(v));
    return r;
}

#define GS_SET_XYZ(x, y, z) ((unsigned long)(x) | ((unsigned long)(y) << 16) | ((unsigned long)(z) << 32))

/* Matching: constcount stand-in (fitted: 1489 float constants precede enDrawRadar) */
static float __stripped_float_code_1(float x) {
    return x +
        3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f +
        35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f +
        67.0f + 69.0f + 71.0f + 73.0f + 75.0f + 77.0f + 79.0f + 81.0f + 83.0f + 85.0f + 87.0f + 89.0f + 91.0f + 93.0f + 95.0f + 97.0f +
        99.0f + 101.0f + 103.0f + 105.0f + 107.0f + 109.0f + 111.0f + 113.0f + 115.0f + 117.0f + 119.0f + 121.0f + 123.0f + 125.0f + 127.0f + 129.0f +
        131.0f + 133.0f + 135.0f + 137.0f + 139.0f + 141.0f + 143.0f + 145.0f + 147.0f + 149.0f + 151.0f + 153.0f + 155.0f + 157.0f + 159.0f + 161.0f +
        163.0f + 165.0f + 167.0f + 169.0f + 171.0f + 173.0f + 175.0f + 177.0f + 179.0f + 181.0f + 183.0f + 185.0f + 187.0f + 189.0f + 191.0f + 193.0f +
        195.0f + 197.0f + 199.0f + 201.0f + 203.0f + 205.0f + 207.0f + 209.0f + 211.0f + 213.0f + 215.0f + 217.0f + 219.0f + 221.0f + 223.0f + 225.0f +
        227.0f + 229.0f + 231.0f + 233.0f + 235.0f + 237.0f + 239.0f + 241.0f + 243.0f + 245.0f + 247.0f + 249.0f + 251.0f + 253.0f + 255.0f + 257.0f +
        259.0f + 261.0f + 263.0f + 265.0f + 267.0f + 269.0f + 271.0f + 273.0f + 275.0f + 277.0f + 279.0f + 281.0f + 283.0f + 285.0f + 287.0f + 289.0f +
        291.0f + 293.0f + 295.0f + 297.0f + 299.0f + 301.0f + 303.0f + 305.0f + 307.0f + 309.0f + 311.0f + 313.0f + 315.0f + 317.0f + 319.0f + 321.0f +
        323.0f + 325.0f + 327.0f + 329.0f + 331.0f + 333.0f + 335.0f + 337.0f + 339.0f + 341.0f + 343.0f + 345.0f + 347.0f + 349.0f + 351.0f + 353.0f +
        355.0f + 357.0f + 359.0f + 361.0f + 363.0f + 365.0f + 367.0f + 369.0f + 371.0f + 373.0f + 375.0f + 377.0f + 379.0f + 381.0f + 383.0f + 385.0f +
        387.0f + 389.0f + 391.0f + 393.0f + 395.0f + 397.0f + 399.0f + 401.0f + 403.0f + 405.0f + 407.0f + 409.0f + 411.0f + 413.0f + 415.0f + 417.0f +
        419.0f + 421.0f + 423.0f + 425.0f + 427.0f + 429.0f + 431.0f + 433.0f + 435.0f + 437.0f + 439.0f + 441.0f + 443.0f + 445.0f + 447.0f + 449.0f +
        451.0f + 453.0f + 455.0f + 457.0f + 459.0f + 461.0f + 463.0f + 465.0f + 467.0f + 469.0f + 471.0f + 473.0f + 475.0f + 477.0f + 479.0f + 481.0f +
        483.0f + 485.0f + 487.0f + 489.0f + 491.0f + 493.0f + 495.0f + 497.0f + 499.0f + 501.0f + 503.0f + 505.0f + 507.0f + 509.0f + 511.0f + 513.0f +
        515.0f + 517.0f + 519.0f + 521.0f + 523.0f + 525.0f + 527.0f + 529.0f + 531.0f + 533.0f + 535.0f + 537.0f + 539.0f + 541.0f + 543.0f + 545.0f +
        547.0f + 549.0f + 551.0f + 553.0f + 555.0f + 557.0f + 559.0f + 561.0f + 563.0f + 565.0f + 567.0f + 569.0f + 571.0f + 573.0f + 575.0f + 577.0f +
        579.0f + 581.0f + 583.0f + 585.0f + 587.0f + 589.0f + 591.0f + 593.0f + 595.0f + 597.0f + 599.0f + 601.0f + 603.0f + 605.0f + 607.0f + 609.0f +
        611.0f + 613.0f + 615.0f + 617.0f + 619.0f + 621.0f + 623.0f + 625.0f + 627.0f + 629.0f + 631.0f + 633.0f + 635.0f + 637.0f + 639.0f + 641.0f +
        643.0f + 645.0f + 647.0f + 649.0f + 651.0f + 653.0f + 655.0f + 657.0f + 659.0f + 661.0f + 663.0f + 665.0f + 667.0f + 669.0f + 671.0f + 673.0f +
        675.0f + 677.0f + 679.0f + 681.0f + 683.0f + 685.0f + 687.0f + 689.0f + 691.0f + 693.0f + 695.0f + 697.0f + 699.0f + 701.0f + 703.0f + 705.0f +
        707.0f + 709.0f + 711.0f + 713.0f + 715.0f + 717.0f + 719.0f + 721.0f + 723.0f + 725.0f + 727.0f + 729.0f + 731.0f + 733.0f + 735.0f + 737.0f +
        739.0f + 741.0f + 743.0f + 745.0f + 747.0f + 749.0f + 751.0f + 753.0f + 755.0f + 757.0f + 759.0f + 761.0f + 763.0f + 765.0f + 767.0f + 769.0f +
        771.0f + 773.0f + 775.0f + 777.0f + 779.0f + 781.0f + 783.0f + 785.0f + 787.0f + 789.0f + 791.0f + 793.0f + 795.0f + 797.0f + 799.0f + 801.0f +
        803.0f + 805.0f + 807.0f + 809.0f + 811.0f + 813.0f + 815.0f + 817.0f + 819.0f + 821.0f + 823.0f + 825.0f + 827.0f + 829.0f + 831.0f + 833.0f +
        835.0f + 837.0f + 839.0f + 841.0f + 843.0f + 845.0f + 847.0f + 849.0f + 851.0f + 853.0f + 855.0f + 857.0f + 859.0f + 861.0f + 863.0f + 865.0f +
        867.0f + 869.0f + 871.0f + 873.0f + 875.0f + 877.0f + 879.0f + 881.0f + 883.0f + 885.0f + 887.0f + 889.0f + 891.0f + 893.0f + 895.0f + 897.0f +
        899.0f + 901.0f + 903.0f + 905.0f + 907.0f + 909.0f + 911.0f + 913.0f + 915.0f + 917.0f + 919.0f + 921.0f + 923.0f + 925.0f + 927.0f + 929.0f +
        931.0f + 933.0f + 935.0f + 937.0f + 939.0f + 941.0f + 943.0f + 945.0f + 947.0f + 949.0f + 951.0f + 953.0f + 955.0f + 957.0f + 959.0f + 961.0f +
        963.0f + 965.0f + 967.0f + 969.0f + 971.0f + 973.0f + 975.0f + 977.0f + 979.0f + 981.0f + 983.0f + 985.0f + 987.0f + 989.0f + 991.0f + 993.0f +
        995.0f + 997.0f + 999.0f + 1001.0f + 1003.0f + 1005.0f + 1007.0f + 1009.0f + 1011.0f + 1013.0f + 1015.0f + 1017.0f + 1019.0f + 1021.0f + 1023.0f + 1025.0f +
        1027.0f + 1029.0f + 1031.0f + 1033.0f + 1035.0f + 1037.0f + 1039.0f + 1041.0f + 1043.0f + 1045.0f + 1047.0f + 1049.0f + 1051.0f + 1053.0f + 1055.0f + 1057.0f +
        1059.0f + 1061.0f + 1063.0f + 1065.0f + 1067.0f + 1069.0f + 1071.0f + 1073.0f + 1075.0f + 1077.0f + 1079.0f + 1081.0f + 1083.0f + 1085.0f + 1087.0f + 1089.0f +
        1091.0f + 1093.0f + 1095.0f + 1097.0f + 1099.0f + 1101.0f + 1103.0f + 1105.0f + 1107.0f + 1109.0f + 1111.0f + 1113.0f + 1115.0f + 1117.0f + 1119.0f + 1121.0f +
        1123.0f + 1125.0f + 1127.0f + 1129.0f + 1131.0f + 1133.0f + 1135.0f + 1137.0f + 1139.0f + 1141.0f + 1143.0f + 1145.0f + 1147.0f + 1149.0f + 1151.0f + 1153.0f +
        1155.0f + 1157.0f + 1159.0f + 1161.0f + 1163.0f + 1165.0f + 1167.0f + 1169.0f + 1171.0f + 1173.0f + 1175.0f + 1177.0f + 1179.0f + 1181.0f + 1183.0f + 1185.0f +
        1187.0f + 1189.0f + 1191.0f + 1193.0f + 1195.0f + 1197.0f + 1199.0f + 1201.0f + 1203.0f + 1205.0f + 1207.0f + 1209.0f + 1211.0f + 1213.0f + 1215.0f + 1217.0f +
        1219.0f + 1221.0f + 1223.0f + 1225.0f + 1227.0f + 1229.0f + 1231.0f + 1233.0f + 1235.0f + 1237.0f + 1239.0f + 1241.0f + 1243.0f + 1245.0f + 1247.0f + 1249.0f +
        1251.0f + 1253.0f + 1255.0f + 1257.0f + 1259.0f + 1261.0f + 1263.0f + 1265.0f + 1267.0f + 1269.0f + 1271.0f + 1273.0f + 1275.0f + 1277.0f + 1279.0f + 1281.0f +
        1283.0f + 1285.0f + 1287.0f + 1289.0f + 1291.0f + 1293.0f + 1295.0f + 1297.0f + 1299.0f + 1301.0f + 1303.0f + 1305.0f + 1307.0f + 1309.0f + 1311.0f + 1313.0f +
        1315.0f + 1317.0f + 1319.0f + 1321.0f + 1323.0f + 1325.0f + 1327.0f + 1329.0f + 1331.0f + 1333.0f + 1335.0f + 1337.0f + 1339.0f + 1341.0f + 1343.0f + 1345.0f +
        1347.0f + 1349.0f + 1351.0f + 1353.0f + 1355.0f + 1357.0f + 1359.0f + 1361.0f + 1363.0f + 1365.0f + 1367.0f + 1369.0f + 1371.0f + 1373.0f + 1375.0f + 1377.0f +
        1379.0f + 1381.0f + 1383.0f + 1385.0f + 1387.0f + 1389.0f + 1391.0f + 1393.0f + 1395.0f + 1397.0f + 1399.0f + 1401.0f + 1403.0f + 1405.0f + 1407.0f + 1409.0f +
        1411.0f + 1413.0f + 1415.0f + 1417.0f + 1419.0f + 1421.0f + 1423.0f + 1425.0f + 1427.0f + 1429.0f + 1431.0f + 1433.0f + 1435.0f + 1437.0f + 1439.0f + 1441.0f +
        1443.0f + 1445.0f + 1447.0f + 1449.0f + 1451.0f + 1453.0f + 1455.0f + 1457.0f + 1459.0f + 1461.0f + 1463.0f + 1465.0f + 1467.0f + 1469.0f + 1471.0f + 1473.0f +
        1475.0f + 1477.0f + 1479.0f + 1481.0f + 1483.0f + 1485.0f + 1487.0f + 1489.0f + 1491.0f + 1493.0f + 1495.0f + 1497.0f + 1499.0f + 1501.0f + 1503.0f + 1505.0f +
        1507.0f + 1509.0f + 1511.0f + 1513.0f + 1515.0f + 1517.0f + 1519.0f + 1521.0f + 1523.0f + 1525.0f + 1527.0f + 1529.0f + 1531.0f + 1533.0f + 1535.0f + 1537.0f +
        1539.0f + 1541.0f + 1543.0f + 1545.0f + 1547.0f + 1549.0f + 1551.0f + 1553.0f + 1555.0f + 1557.0f + 1559.0f + 1561.0f + 1563.0f + 1565.0f + 1567.0f + 1569.0f +
        1571.0f + 1573.0f + 1575.0f + 1577.0f + 1579.0f + 1581.0f + 1583.0f + 1585.0f + 1587.0f + 1589.0f + 1591.0f + 1593.0f + 1595.0f + 1597.0f + 1599.0f + 1601.0f +
        1603.0f + 1605.0f + 1607.0f + 1609.0f + 1611.0f + 1613.0f + 1615.0f + 1617.0f + 1619.0f + 1621.0f + 1623.0f + 1625.0f + 1627.0f + 1629.0f + 1631.0f + 1633.0f +
        1635.0f + 1637.0f + 1639.0f + 1641.0f + 1643.0f + 1645.0f + 1647.0f + 1649.0f + 1651.0f + 1653.0f + 1655.0f + 1657.0f + 1659.0f + 1661.0f + 1663.0f + 1665.0f +
        1667.0f + 1669.0f + 1671.0f + 1673.0f + 1675.0f + 1677.0f + 1679.0f + 1681.0f + 1683.0f + 1685.0f + 1687.0f + 1689.0f + 1691.0f + 1693.0f + 1695.0f + 1697.0f +
        1699.0f + 1701.0f + 1703.0f + 1705.0f + 1707.0f + 1709.0f + 1711.0f + 1713.0f + 1715.0f + 1717.0f + 1719.0f + 1721.0f + 1723.0f + 1725.0f + 1727.0f + 1729.0f +
        1731.0f + 1733.0f + 1735.0f + 1737.0f + 1739.0f + 1741.0f + 1743.0f + 1745.0f + 1747.0f + 1749.0f + 1751.0f + 1753.0f + 1755.0f + 1757.0f + 1759.0f + 1761.0f +
        1763.0f + 1765.0f + 1767.0f + 1769.0f + 1771.0f + 1773.0f + 1775.0f + 1777.0f + 1779.0f + 1781.0f + 1783.0f + 1785.0f + 1787.0f + 1789.0f + 1791.0f + 1793.0f +
        1795.0f + 1797.0f + 1799.0f + 1801.0f + 1803.0f + 1805.0f + 1807.0f + 1809.0f + 1811.0f + 1813.0f + 1815.0f + 1817.0f + 1819.0f + 1821.0f + 1823.0f + 1825.0f +
        1827.0f + 1829.0f + 1831.0f + 1833.0f + 1835.0f + 1837.0f + 1839.0f + 1841.0f + 1843.0f + 1845.0f + 1847.0f + 1849.0f + 1851.0f + 1853.0f + 1855.0f + 1857.0f +
        1859.0f + 1861.0f + 1863.0f + 1865.0f + 1867.0f + 1869.0f + 1871.0f + 1873.0f + 1875.0f + 1877.0f + 1879.0f + 1881.0f + 1883.0f + 1885.0f + 1887.0f + 1889.0f +
        1891.0f + 1893.0f + 1895.0f + 1897.0f + 1899.0f + 1901.0f + 1903.0f + 1905.0f + 1907.0f + 1909.0f + 1911.0f + 1913.0f + 1915.0f + 1917.0f + 1919.0f + 1921.0f +
        1923.0f + 1925.0f + 1927.0f + 1929.0f + 1931.0f + 1933.0f + 1935.0f + 1937.0f + 1939.0f + 1941.0f + 1943.0f + 1945.0f + 1947.0f + 1949.0f + 1951.0f + 1953.0f +
        1955.0f + 1957.0f + 1959.0f + 1961.0f + 1963.0f + 1965.0f + 1967.0f + 1969.0f + 1971.0f + 1973.0f + 1975.0f + 1977.0f + 1979.0f + 1981.0f + 1983.0f + 1985.0f +
        1987.0f + 1989.0f + 1991.0f + 1993.0f + 1995.0f + 1997.0f + 1999.0f + 2001.0f + 2003.0f + 2005.0f + 2007.0f + 2009.0f + 2011.0f + 2013.0f + 2015.0f + 2017.0f +
        2019.0f + 2021.0f + 2023.0f + 2025.0f + 2027.0f + 2029.0f + 2031.0f + 2033.0f + 2035.0f + 2037.0f + 2039.0f + 2041.0f + 2043.0f + 2045.0f + 2047.0f + 2049.0f +
        2051.0f + 2053.0f + 2055.0f + 2057.0f + 2059.0f + 2061.0f + 2063.0f + 2065.0f + 2067.0f + 2069.0f + 2071.0f + 2073.0f + 2075.0f + 2077.0f + 2079.0f + 2081.0f +
        2083.0f + 2085.0f + 2087.0f + 2089.0f + 2091.0f + 2093.0f + 2095.0f + 2097.0f + 2099.0f + 2101.0f + 2103.0f + 2105.0f + 2107.0f + 2109.0f + 2111.0f + 2113.0f +
        2115.0f + 2117.0f + 2119.0f + 2121.0f + 2123.0f + 2125.0f + 2127.0f + 2129.0f + 2131.0f + 2133.0f + 2135.0f + 2137.0f + 2139.0f + 2141.0f + 2143.0f + 2145.0f +
        2147.0f + 2149.0f + 2151.0f + 2153.0f + 2155.0f + 2157.0f + 2159.0f + 2161.0f + 2163.0f + 2165.0f + 2167.0f + 2169.0f + 2171.0f + 2173.0f + 2175.0f + 2177.0f +
        2179.0f + 2181.0f + 2183.0f + 2185.0f + 2187.0f + 2189.0f + 2191.0f + 2193.0f + 2195.0f + 2197.0f + 2199.0f + 2201.0f + 2203.0f + 2205.0f + 2207.0f + 2209.0f +
        2211.0f + 2213.0f + 2215.0f + 2217.0f + 2219.0f + 2221.0f + 2223.0f + 2225.0f + 2227.0f + 2229.0f + 2231.0f + 2233.0f + 2235.0f + 2237.0f + 2239.0f + 2241.0f +
        2243.0f + 2245.0f + 2247.0f + 2249.0f + 2251.0f + 2253.0f + 2255.0f + 2257.0f + 2259.0f + 2261.0f + 2263.0f + 2265.0f + 2267.0f + 2269.0f + 2271.0f + 2273.0f +
        2275.0f + 2277.0f + 2279.0f + 2281.0f + 2283.0f + 2285.0f + 2287.0f + 2289.0f + 2291.0f + 2293.0f + 2295.0f + 2297.0f + 2299.0f + 2301.0f + 2303.0f + 2305.0f +
        2307.0f + 2309.0f + 2311.0f + 2313.0f + 2315.0f + 2317.0f + 2319.0f + 2321.0f + 2323.0f + 2325.0f + 2327.0f + 2329.0f + 2331.0f + 2333.0f + 2335.0f + 2337.0f +
        2339.0f + 2341.0f + 2343.0f + 2345.0f + 2347.0f + 2349.0f + 2351.0f + 2353.0f + 2355.0f + 2357.0f + 2359.0f + 2361.0f + 2363.0f + 2365.0f + 2367.0f + 2369.0f +
        2371.0f + 2373.0f + 2375.0f + 2377.0f + 2379.0f + 2381.0f + 2383.0f + 2385.0f + 2387.0f + 2389.0f + 2391.0f + 2393.0f + 2395.0f + 2397.0f + 2399.0f + 2401.0f +
        2403.0f + 2405.0f + 2407.0f + 2409.0f + 2411.0f + 2413.0f + 2415.0f + 2417.0f + 2419.0f + 2421.0f + 2423.0f + 2425.0f + 2427.0f + 2429.0f + 2431.0f + 2433.0f +
        2435.0f + 2437.0f + 2439.0f + 2441.0f + 2443.0f + 2445.0f + 2447.0f + 2449.0f + 2451.0f + 2453.0f + 2455.0f + 2457.0f + 2459.0f + 2461.0f + 2463.0f + 2465.0f +
        2467.0f + 2469.0f + 2471.0f + 2473.0f + 2475.0f + 2477.0f + 2479.0f + 2481.0f + 2483.0f + 2485.0f + 2487.0f + 2489.0f + 2491.0f + 2493.0f + 2495.0f + 2497.0f +
        2499.0f + 2501.0f + 2503.0f + 2505.0f + 2507.0f + 2509.0f + 2511.0f + 2513.0f + 2515.0f + 2517.0f + 2519.0f + 2521.0f + 2523.0f + 2525.0f + 2527.0f + 2529.0f +
        2531.0f + 2533.0f + 2535.0f + 2537.0f + 2539.0f + 2541.0f + 2543.0f + 2545.0f + 2547.0f + 2549.0f + 2551.0f + 2553.0f + 2555.0f + 2557.0f + 2559.0f + 2561.0f +
        2563.0f + 2565.0f + 2567.0f + 2569.0f + 2571.0f + 2573.0f + 2575.0f + 2577.0f + 2579.0f + 2581.0f + 2583.0f + 2585.0f + 2587.0f + 2589.0f + 2591.0f + 2593.0f +
        2595.0f + 2597.0f + 2599.0f + 2601.0f + 2603.0f + 2605.0f + 2607.0f + 2609.0f + 2611.0f + 2613.0f + 2615.0f + 2617.0f + 2619.0f + 2621.0f + 2623.0f + 2625.0f +
        2627.0f + 2629.0f + 2631.0f + 2633.0f + 2635.0f + 2637.0f + 2639.0f + 2641.0f + 2643.0f + 2645.0f + 2647.0f + 2649.0f + 2651.0f + 2653.0f + 2655.0f + 2657.0f +
        2659.0f + 2661.0f + 2663.0f + 2665.0f + 2667.0f + 2669.0f + 2671.0f + 2673.0f + 2675.0f + 2677.0f + 2679.0f + 2681.0f + 2683.0f + 2685.0f + 2687.0f + 2689.0f +
        2691.0f + 2693.0f + 2695.0f + 2697.0f + 2699.0f + 2701.0f + 2703.0f + 2705.0f + 2707.0f + 2709.0f + 2711.0f + 2713.0f + 2715.0f + 2717.0f + 2719.0f + 2721.0f +
        2723.0f + 2725.0f + 2727.0f + 2729.0f + 2731.0f + 2733.0f + 2735.0f + 2737.0f + 2739.0f + 2741.0f + 2743.0f + 2745.0f + 2747.0f + 2749.0f + 2751.0f + 2753.0f +
        2755.0f + 2757.0f + 2759.0f + 2761.0f + 2763.0f + 2765.0f + 2767.0f + 2769.0f + 2771.0f + 2773.0f + 2775.0f + 2777.0f + 2779.0f + 2781.0f + 2783.0f + 2785.0f +
        2787.0f + 2789.0f + 2791.0f + 2793.0f + 2795.0f + 2797.0f + 2799.0f + 2801.0f + 2803.0f + 2805.0f + 2807.0f + 2809.0f + 2811.0f + 2813.0f + 2815.0f + 2817.0f +
        2819.0f + 2821.0f + 2823.0f + 2825.0f + 2827.0f + 2829.0f + 2831.0f + 2833.0f + 2835.0f + 2837.0f + 2839.0f + 2841.0f + 2843.0f + 2845.0f + 2847.0f + 2849.0f +
        2851.0f + 2853.0f + 2855.0f + 2857.0f + 2859.0f + 2861.0f + 2863.0f + 2865.0f + 2867.0f + 2869.0f + 2871.0f + 2873.0f + 2875.0f + 2877.0f + 2879.0f + 2881.0f +
        2883.0f + 2885.0f + 2887.0f + 2889.0f + 2891.0f + 2893.0f + 2895.0f + 2897.0f + 2899.0f + 2901.0f + 2903.0f + 2905.0f + 2907.0f + 2909.0f + 2911.0f + 2913.0f +
        2915.0f + 2917.0f + 2919.0f + 2921.0f + 2923.0f + 2925.0f + 2927.0f + 2929.0f + 2931.0f + 2933.0f + 2935.0f + 2937.0f + 2939.0f + 2941.0f + 2943.0f + 2945.0f +
        2947.0f + 2949.0f + 2951.0f + 2953.0f + 2955.0f + 2957.0f + 2959.0f + 2961.0f + 2963.0f + 2965.0f + 2967.0f + 2969.0f + 2971.0f + 2973.0f + 2975.0f + 2977.0f +
        2979.0f;
}

static void enDrawWall(float *wall_pos1, float *wall_pos2, float *ppos, float prot);
static void enDrawColumn(float *column_pos, float size, float *ppos, float prot);
static void enDrawTriangle(float *pos, float rot, unsigned int color, float size);
static void enDrawBox(float *pos, float rot, unsigned int color, float size1, float size2);

static float view_rate;

/** Draws the radar: enemies as triangles or boxes, then the nearby collision walls and
 * columns, rotated to the player's (or the room's) view. Does nothing while the radar is off. */
void enDrawRadar(void) {
    struct EnLOCAL_DATA *dp;
    int i;
    float *ppos;
    float pr;
    float vec[4];
    float rot;
    unsigned long c;
    void **cl_list;
    void *list_backup;
    struct _CL_CLDHEADER *ch;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_PLANE *pl;
    struct _CL_HITPOLY_COLUMN *clmn;
    struct _CL_HITPOLY_COLUMN *pl2;
    int *ptr;
    int act;
    int usemax;
    int kind;
    int n;
    struct SubCharacter *mp;

    ppos = (float *)GetPlayerInfoForCameraCtrl();
    if (!playing.radar || ((Sh2sys.main_status >> 6) & 1)) {
        return;
    }
    view_rate = (playing.radar & 1) ? 60.0f : 90.0f;
    if (playing.radar <= 2) {
        pr = -ppos[5] - PI / 2;
    } else {
        switch (RoomNameJms()) {
        case 1:
            pr = PI;
            break;
        default:
            pr = -PI / 2;
            break;
        }
    }
    spkOpenDGiftag(0x1000000000008000, 0xE, -2, 0);
    PK_ADD(0x44);
    PK_ADD(0x42);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0xF400000000008000, 0x055555555D105D10);
    PK_ADD(0x46);
    PK_ADD(0x3F80000010000000);
    PK_ADD(0x74D08790);
    PK_ADD(0x79B08C70);
    PK_ADD(0x42);
    PK_ADD(0x3F80000040808080);
    PK_ADD(0x74C08780);
    PK_ADD(0x74C08C80);
    PK_ADD(0x79C08C80);
    PK_ADD(0x79C08780);
    PK_ADD(0x74B08780);
    PK_ADD(0x74B08C90);
    PK_ADD(0x79D08C90);
    PK_ADD(0x79D08770);
    PK_ADD(0x74B08770);
    spkCloseOpenDGiftag(0x1000000000008000, 0xE);
    PK_ADD(0x009B004D01C70179);
    PK_ADD(0x40);
    spkCloseGiftag();
    dp = enLocalWork.Data;
    for (i = 0; i < 32; i++, dp++) {
        if (!dp->kind) {
            continue;
        }
        if (!(dp->scp->status & 0x10)) {
            continue;
        }
        _shSubVector(vec, (float *)&dp->scp->pos, ppos);
        if (fabsf(vec[0]) <= 750.0f * view_rate && fabsf(vec[2]) <= 750.0f * view_rate) {
            shRotVectorY(vec, vec, pr);
            rot = dp->scp->rot.y + pr;
            if (dp->scp->battle.status & 2) {
                c = 0x60101010;
            } else {
                c = 0x4030FF30;
            }
            switch (dp->kind) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 9:
            case 10:
                enDrawTriangle(vec, rot, (unsigned int)c, dp->size);
                break;
            case 7:
                enDrawBox(vec, rot, (unsigned int)c, 1.2f * dp->size, dp->size);
                break;
            case 8:
                enDrawBox(vec, rot, (unsigned int)c, dp->size * 0.5f, dp->size * 0.8f);
                break;
            case 11:
                enDrawBox(vec, rot, (unsigned int)c, dp->size * 0.8f, dp->size * 0.5f);
                break;
            case 15:
                enDrawTriangle(vec, rot, (unsigned int)c, 100.0f);
                break;
            case 12:
                break;
            default:
                enDrawTriangle(vec, rot, 0, dp->size);
                break;
            }
        }
    }
    if (playing.radar <= 2) {
        rot = -PI / 2;
    } else {
        rot = ppos[5] + pr;
    }
    c = 0x4030FF30;
    vzero(vec);
    enDrawTriangle(vec, rot, (unsigned int)c, 200.0f);
    mp = NULL;
    list_backup = cl_list = loadBgCLD_GetLoadedDataAddrList();
    spkOpenDGiftag(0x4400000000008000, 0x5D10, -3, 4);
    while ((ch = *cl_list++)) {
        if (ch->disable) {
            continue;
        }
        wall = (struct _CL_HITPOLY_PLANE *)((char *)ch + ch->wldofs);
        for (n = 0; n < 16; n++) {
            ptr = (int *)((char *)ch + ch->b1ofs[n]);
            while (*ptr != -1) {
                pl = &wall[*ptr];
                if (pl->shape == 0) {
                    enDrawWall(pl->p[0], pl->p[1], ppos, pr);
                    enDrawWall(pl->p[1], pl->p[2], ppos, pr);
                    enDrawWall(pl->p[2], pl->p[0], ppos, pr);
                } else {
                    enDrawWall(pl->p[0], pl->p[1], ppos, pr);
                    enDrawWall(pl->p[1], pl->p[2], ppos, pr);
                    enDrawWall(pl->p[2], pl->p[3], ppos, pr);
                    enDrawWall(pl->p[3], pl->p[0], ppos, pr);
                }
                ptr++;
            }
        }
        wall = (struct _CL_HITPOLY_PLANE *)((char *)ch + ch->swdofs);
        for (n = 0; n < 16; n++) {
            ptr = (int *)((char *)ch + ch->b3ofs[n]);
            while (*ptr != -1) {
                pl = &wall[*ptr];
                if (pl->shape == 0) {
                    enDrawWall(pl->p[0], pl->p[1], ppos, pr);
                    enDrawWall(pl->p[1], pl->p[2], ppos, pr);
                    enDrawWall(pl->p[2], pl->p[0], ppos, pr);
                } else {
                    enDrawWall(pl->p[0], pl->p[1], ppos, pr);
                    enDrawWall(pl->p[1], pl->p[2], ppos, pr);
                    enDrawWall(pl->p[2], pl->p[3], ppos, pr);
                    enDrawWall(pl->p[3], pl->p[0], ppos, pr);
                }
                ptr++;
            }
        }
    }
    act = clDynamicWallListAct ? 0 : 1;
    for (i = 0; i < clDynamicWallList[act].use; i++) {
        pl = clDynamicWallList[act].dw[i];
        while (pl->kind) {
            if (pl->shape == 0) {
                enDrawWall(pl->p[0], pl->p[1], ppos, pr);
                enDrawWall(pl->p[1], pl->p[2], ppos, pr);
                enDrawWall(pl->p[2], pl->p[0], ppos, pr);
            } else {
                enDrawWall(pl->p[0], pl->p[1], ppos, pr);
                enDrawWall(pl->p[1], pl->p[2], ppos, pr);
                enDrawWall(pl->p[2], pl->p[3], ppos, pr);
                enDrawWall(pl->p[3], pl->p[0], ppos, pr);
            }
            pl++;
        }
    }
    spkCloseGiftag();
    spkOpenDGiftag(0x6400000000008000, 0x555D10, -3, 4);
    cl_list = list_backup;
    while ((ch = *cl_list++)) {
        if (ch->disable) {
            continue;
        }
        clmn = (struct _CL_HITPOLY_COLUMN *)((char *)ch + ch->cldofs);
        for (n = 0; n < 16; n++) {
            ptr = (int *)((char *)ch + ch->clofs[n]);
            while (*ptr != -1) {
                pl2 = &clmn[*ptr];
                enDrawColumn(pl2->p[0], pl2->p[1][3], ppos, pr);
                ptr++;
            }
        }
    }
    act = clCharaListAct ? 0 : 1;
    usemax = clCharaListUse[act];
    for (i = 0; i < usemax; i++) {
        kind = clCharaList[act][i].sc->kind;
        if ((kind >= 0x100 && kind <= 0x103) || (kind >= 0x200 && kind <= 0x20B)) {
            continue;
        }
        if (kind == 0x105) {
            mp = clCharaList[act][i].sc;
            continue;
        }
        enDrawColumn(clCharaList[act][i].col.p[0], clCharaList[act][i].col.p[1][3], ppos, pr);
    }
    spkCloseGiftag();
    if (mp) {
        _shSubVector(vec, (float *)&mp->pos, ppos);
        if (fabsf(vec[0]) <= 750.0f * view_rate && fabsf(vec[2]) <= 750.0f * view_rate) {
            shRotVectorY(vec, vec, pr);
            rot = mp->rot.y + pr;
            enDrawTriangle(vec, rot, 0x40C030C0, 150.0f);
        }
    }
    spkOpenDGiftag(0x1000000000008000, 0xE, -5, 0);
    PK_ADD(0x01FF000001FF0000);
    PK_ADD(0x40);
    spkCloseGiftag();
}

static void enDrawWall(float *wall_pos1, float *wall_pos2, float *ppos, float prot) {
    float vec[4];
    float vec2[4];
    int scr[4];
    unsigned long xyz;

    if (wall_pos1[0] == wall_pos2[0] && wall_pos1[2] == wall_pos2[2]) {
        return;
    }
    vzero(scr);
    _shSubVector(vec, wall_pos1, ppos);
    _shSubVector(vec2, wall_pos2, ppos);
    shRotVectorY(vec, vec, prot);
    scr[2] = 0xFFFFFE;
    scr[0] = ftoi4(2048.0f + vec[2] / view_rate) + 0xA00;
    scr[1] = ftoi4(2048.0f + 1.3333334f * (vec[0] / view_rate)) - 0x8C0;
    if (scr[0] < 0 || scr[1] < 0 || scr[0] > 0xFFFF || scr[1] > 0xFFFF) {
        return;
    }
    xyz = _shPackXYZ(scr);
    shRotVectorY(vec2, vec2, prot);
    scr[0] = ftoi4(2048.0f + vec2[2] / view_rate) + 0xA00;
    scr[1] = ftoi4(2048.0f + 1.3333334f * (vec2[0] / view_rate)) - 0x8C0;
    if (scr[0] < 0 || scr[1] < 0 || scr[0] > 0xFFFF || scr[1] > 0xFFFF) {
        return;
    }
    PK_ADD(0x41);
    if ((vec[1] > 250.0f && vec2[1] > 250.0f) || (vec[1] < -250.0f && vec2[1] < -250.0f)) {
        PK_ADD(0x2040C000);
    } else {
        PK_ADD(0x40FF5000);
    }
    PK_ADD(xyz);
    PK_ADD(_shPackXYZ(scr));
}

static void enDrawColumn(float *column_pos, float size, float *ppos, float prot) {
    float vec[4];
    float vec2[4];
    int scr[4];
    unsigned long xyz1;
    unsigned long xyz2;
    unsigned long xyz3;

    vzero(scr);
    scr[2] = 0xFFFFFE;
    _shSubVector(vec, column_pos, ppos);
    vcopy(vec, vec2);
    vec2[0] -= size;
    shRotVectorY(vec2, vec2, prot);
    scr[0] = ftoi4(2048.0f + vec2[2] / view_rate) + 0xA00;
    scr[1] = ftoi4(2048.0f + 1.3333334f * (vec2[0] / view_rate)) - 0x8C0;
    if (scr[0] < 0 || scr[1] < 0 || scr[0] > 0xFFFF || scr[1] > 0xFFFF) {
        return;
    }
    xyz1 = _shPackXYZ(scr);
    vcopy(vec, vec2);
    vec2[2] += size;
    shRotVectorY(vec2, vec2, prot);
    scr[0] = ftoi4(2048.0f + vec2[2] / view_rate) + 0xA00;
    scr[1] = ftoi4(2048.0f + 1.3333334f * (vec2[0] / view_rate)) - 0x8C0;
    if (scr[0] < 0 || scr[1] < 0 || scr[0] > 0xFFFF || scr[1] > 0xFFFF) {
        return;
    }
    xyz2 = _shPackXYZ(scr);
    vcopy(vec, vec2);
    vec2[2] -= size;
    shRotVectorY(vec2, vec2, prot);
    scr[0] = ftoi4(2048.0f + vec2[2] / view_rate) + 0xA00;
    scr[1] = ftoi4(2048.0f + 1.3333334f * (vec2[0] / view_rate)) - 0x8C0;
    if (scr[0] < 0 || scr[1] < 0 || scr[0] > 0xFFFF || scr[1] > 0xFFFF) {
        return;
    }
    xyz3 = _shPackXYZ(scr);
    vcopy(vec, vec2);
    vec2[0] += size;
    shRotVectorY(vec2, vec2, prot);
    scr[0] = ftoi4(2048.0f + vec2[2] / view_rate) + 0xA00;
    scr[1] = ftoi4(2048.0f + 1.3333334f * (vec2[0] / view_rate)) - 0x8C0;
    if (scr[0] < 0 || scr[1] < 0 || scr[0] > 0xFFFF || scr[1] > 0xFFFF) {
        return;
    }
    PK_ADD(0x44);
    PK_ADD(0x40FF5000);
    PK_ADD(xyz1);
    PK_ADD(xyz2);
    PK_ADD(xyz3);
    PK_ADD(_shPackXYZ(scr));
}

static void enDrawTriangle(float *pos, float rot, unsigned int color, float size) {
    float x;
    float y;
    float dx;
    float dy;

    x = pos[2];
    y = pos[0];
    dx = size * shCosF(rot);
    dy = size * shSinF(rot);
    spkOpenDGiftag(0x5400000000008000, 0x5DD10, -4, 4);
    PK_ADD(0x43);
    PK_ADD(color);
    PK_ADD(GS_SET_XYZ(ftoi4(2048.0f + (x + dx) / view_rate) + 0xA00,
                      ftoi4(2048.0f + 1.3333334f * ((y + dy) / view_rate)) - 0x8C0, 0xFFFFFF));
    PK_ADD(GS_SET_XYZ(ftoi4(2048.0f + (x - dx + dy) / view_rate) + 0xA00,
                      ftoi4(2048.0f + 1.3333334f * ((y - dy - dx) / view_rate)) - 0x8C0, 0xFFFFFF));
    PK_ADD(GS_SET_XYZ(ftoi4(2048.0f + (x - dx - dy) / view_rate) + 0xA00,
                      ftoi4(2048.0f + 1.3333334f * ((y - dy + dx) / view_rate)) - 0x8C0, 0xFFFFFF));
    spkCloseGiftag();
}

static void enDrawBox(float *pos, float rot, unsigned int color, float size1, float size2) {
    float x;
    float y;
    float dx;
    float dy;

    x = pos[2];
    y = pos[0];
    dx = shCosF(rot);
    dy = shSinF(rot);
    spkOpenDGiftag(0x6400000000008000, 0x55DD10, -4, 4);
    PK_ADD(0x44);
    PK_ADD(color);
    PK_ADD(GS_SET_XYZ(ftoi4(2048.0f + (x + dx * size2 + dy * size1) / view_rate) + 0xA00,
                      ftoi4(2048.0f + 1.3333334f * ((y + dy * size2 - dx * size1) / view_rate)) - 0x8C0, 0xFFFFFF));
    PK_ADD(GS_SET_XYZ(ftoi4(2048.0f + (x + dx * size2 - dy * size1) / view_rate) + 0xA00,
                      ftoi4(2048.0f + 1.3333334f * ((y + dy * size2 + dx * size1) / view_rate)) - 0x8C0, 0xFFFFFF));
    PK_ADD(GS_SET_XYZ(ftoi4(2048.0f + (x - dx * size2 + dy * size1) / view_rate) + 0xA00,
                      ftoi4(2048.0f + 1.3333334f * ((y - dy * size2 - dx * size1) / view_rate)) - 0x8C0, 0xFFFFFF));
    PK_ADD(GS_SET_XYZ(ftoi4(2048.0f + (x - dx * size2 - dy * size1) / view_rate) + 0xA00,
                      ftoi4(2048.0f + 1.3333334f * ((y - dy * size2 + dx * size1) / view_rate)) - 0x8C0, 0xFFFFFF));
    spkCloseGiftag();
}
