#pragma once

struct ArrayData {
    int meshID;
    int num;  // is 0 when Attribute not found by readAttribute
    vector v0;
    vector v1;
    vector v2;
    vector v3;
    vector v4;
    vector v5;
    vector v6;
    vector v7;
    vector v8;
    vector v9;
    vector v10;
    vector v11;
    vector v12;
    vector v13;
    vector v14;
    vector v15;
};

struct ClosestData {
    int meshID;
    int prim;
    float u;
    float v;
};
