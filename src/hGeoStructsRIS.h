#pragma once

#define ARRAY_DATA_IDS(name) \
	k_##name##_ArrayData, \
		k_##name##_meshID, \
		k_##name##_num, \
		k_##name##_v0, \
		k_##name##_v1, \
		k_##name##_v2, \
		k_##name##_v3, \
		k_##name##_v4, \
		k_##name##_v5, \
		k_##name##_v6, \
		k_##name##_v7, \
	k_##name##_ArrayDataEnd,


#define ARRAY_DATA_STRUCT(name, mode) \
	RixSCParamInfo(RtUString("ArrayData"),   RtUString(name), k_RixSCStructBegin, mode), \
		RixSCParamInfo(RtUString("meshID"),   k_RixSCInteger,  mode), \
		RixSCParamInfo(RtUString("num"),      k_RixSCInteger,  mode), \
		RixSCParamInfo(RtUString("v0"),       k_RixSCFloat3,   mode), \
		RixSCParamInfo(RtUString("v1"),       k_RixSCFloat3,   mode), \
		RixSCParamInfo(RtUString("v2"),       k_RixSCFloat3,   mode), \
		RixSCParamInfo(RtUString("v3"),       k_RixSCFloat3,   mode), \
		RixSCParamInfo(RtUString("v4"),       k_RixSCFloat3,   mode), \
		RixSCParamInfo(RtUString("v5"),       k_RixSCFloat3,   mode), \
		RixSCParamInfo(RtUString("v6"),       k_RixSCFloat3,   mode), \
		RixSCParamInfo(RtUString("v7"),       k_RixSCFloat3,   mode), \
	RixSCParamInfo(RtUString("ArrayData"),   RtUString(name), k_RixSCStructEnd, mode),

#define ARRAY_DATA_IN(name) ARRAY_DATA_STRUCT(name, k_RixSCInput)
#define ARRAY_DATA_OUT(name) ARRAY_DATA_STRUCT(name, k_RixSCOutput)



#define CLOSEST_DATA_IDS \
	k_ClosestData, \
		k_meshID, \
		k_prim, \
		k_u, \
		k_v, \
	k_ClosestDataEnd,


#define CLOSEST_DATA_STRUCT(name, mode) \
	RixSCParamInfo(RtUString("ClosestData"),   RtUString(name), k_RixSCStructBegin, mode), \
		RixSCParamInfo(RtUString("meshID"),    k_RixSCInteger, mode), \
		RixSCParamInfo(RtUString("prim"),      k_RixSCInteger, mode), \
		RixSCParamInfo(RtUString("u"),         k_RixSCFloat,   mode), \
		RixSCParamInfo(RtUString("v"),         k_RixSCFloat,   mode), \
	RixSCParamInfo(RtUString("ClosestData"),   RtUString(name), k_RixSCStructEnd, mode),

#define CLOSEST_DATA_IN(name) CLOSEST_DATA_STRUCT(name, k_RixSCInput)
#define CLOSEST_DATA_OUT(name) CLOSEST_DATA_STRUCT(name, k_RixSCOutput)
