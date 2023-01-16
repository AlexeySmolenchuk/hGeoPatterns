#pragma once

struct ArrayData {
    int meshID;
    int num;
    vector v0;
    vector v1;
    vector v2;
    vector v3;
    vector v4;
    vector v5;
    vector v6;
    vector v7;
};

struct ClosestData {
    int meshID;
    int prim;
    float u;
    float v;
};
