#include "RixPattern.h"
#include "RixPredefinedStrings.hpp"

#include <GU/GU_Detail.h>
#include <GU/GU_PrimVDB.h>
#include <GU/GU_PrimVolume.h>

#include <map>
#include <iostream>

class sampleVolume: public RixPattern
{
public:
	enum paramId
	{
		// outputs
		k_value,

		// inputs
		k_position,
		k_filename,
		k_primname,
		k_primnumber,
		k_coordsys,

		// end of list
		k_numParams
	};

	struct Data
	{
		GEO_Primitive* volume;
		RtUString coordsys;
	};

	int Init(RixContext &ctx, RtUString const pluginpath) override;

	RixSCParamInfo const *GetParamTable() override;

	void Synchronize(
		RixContext&,
		RixSCSyncMsg,
		RixParameterList const*) override
		{
		}

	void CreateInstanceData(RixContext&,
							RtUString const,
							RixParameterList const*,
							InstanceData*) override;

	void Finalize(RixContext &ctx) override;

	int ComputeOutputParams(RixShadingContext const *sCtx,
							int *nOutputs,
							RixPattern::OutputSpec **outputs,
							RtPointer instanceData,
							RixSCParamInfo const *instanceTable) override;

	bool Bake2dOutput(  RixBakeContext const*,
						Bake2dSpec&,
						RtPointer) override
	{
		return false;
	}

	bool Bake3dOutput(  RixBakeContext const*,
						Bake3dSpec&,
						RtPointer) override
	{
		return false;
	}

private:
	std::unordered_map<RtUString, GU_Detail*> m_geo;
};


int
sampleVolume::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	return 0;
}


void
sampleVolume::Finalize(RixContext &ctx)
{
	for (auto item: m_geo){
		delete item.second;
	}
	PIXAR_ARGUSED(ctx);
}


RixSCParamInfo const*
sampleVolume::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs
		RixSCParamInfo(RtUString("Value"), k_RixSCColor, k_RixSCOutput),

		// inputs
		RixSCParamInfo(RtUString("position"), k_RixSCColor),
		RixSCParamInfo(RtUString("filename"), k_RixSCString),
		RixSCParamInfo(RtUString("name"), k_RixSCString),
		RixSCParamInfo(RtUString("number"), k_RixSCInteger),
		RixSCParamInfo(RtUString("coordsys"), k_RixSCString),

		// end of table
		RixSCParamInfo()
	};
	return &s_ptable[0];
}


void sampleVolume::CreateInstanceData(RixContext& ctx,
										RtUString const handle,
										RixParameterList const* params,
										InstanceData* instanceData)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(handle);

	instanceData->datalen = sizeof(Data);
	instanceData->data = malloc(instanceData->datalen);
	instanceData->freefunc = free;
	Data *data = static_cast<Data*>(instanceData->data);

	RtUString filename = US_NULL;
	params->EvalParam(k_filename, -1, &filename);

	RtUString name("density");
	params->EvalParam(k_primname, -1, &name);
	if (name.Empty())
		name = RtUString("*");

	int number = 0;
	params->EvalParam(k_primnumber, -1, &number);

	data->coordsys = Rix::k_object;
	params->EvalParam(k_coordsys, -1, &data->coordsys);
	if (data->coordsys.Empty())
		data->coordsys = Rix::k_object;

	data->volume = nullptr;

	if (filename.Empty())
		return;

	GU_Detail * gdp;

	auto it = m_geo.find(filename);
	if (it == m_geo.end())
	{
		gdp = new GU_Detail;
		if (gdp->load(filename.CStr()).success())
		{
			m_geo[filename] = gdp;
			// std::cout << "Loaded: " << filename.CStr() << " " << gdp->getMemoryUsage(true) <<std::endl;
		}
		else
		{
			return;
		}
	}
	else
	{
		gdp = it->second;
	}

	if (gdp->findAttribute(GA_AttributeOwner::GA_ATTRIB_PRIMITIVE, "name"))
	{
		GEO_Primitive* found;
		found = gdp->findPrimitiveByName(name.CStr(), GEO_PrimTypeCompat::GEOPRIMVOLUME | GEO_PrimTypeCompat::GEOPRIMVDB, "name", number);
		if (found)
			data->volume = found;
	}
	else if (strcmp(name.CStr(), "*") == 0)
	{
		GA_Primitive *prim;
		GA_FOR_ALL_PRIMITIVES(gdp, prim)
		{
			if (prim->getTypeId() == GEO_PRIMVDB || prim->getTypeId() == GEO_PRIMVOLUME)
			{
				if (number==0)
				{
					data->volume = (GEO_Primitive*)prim;
					break;
				}
				number--;
			}
		}
	}

	return;
}


int
sampleVolume::ComputeOutputParams(RixShadingContext const *sCtx,
									int *nOutputs,
									OutputSpec **outputs,
									RtPointer instanceData,
									RixSCParamInfo const *instanceTable)
{

	PIXAR_ARGUSED(instanceTable);

	Data const* data = static_cast<Data const*>(instanceData);

	if (!data->volume)
		return 1;

	RixSCType type;
	RixSCConnectionInfo cinfo;
	RixSCParamInfo const* paramTable = GetParamTable();
	int numOutputs = -1;
	while (paramTable[++numOutputs].access == k_RixSCOutput) {}

	RixShadingContext::Allocator pool(sCtx);
	OutputSpec *out = *outputs = pool.AllocForPattern<OutputSpec>(numOutputs);
	*nOutputs = numOutputs;

	// looping through the different output ids
	for (int i = 0; i < numOutputs; ++i)
	{
		out[i].paramId = i;
		out[i].detail = k_RixSCInvalidDetail;
		out[i].value = NULL;

		type = paramTable[i].type; // we know this

		sCtx->GetParamInfo(i, &type, &cinfo);

		if( type == k_RixSCColor )
		{
			out[i].detail = k_RixSCVarying;
			out[i].value = pool.AllocForPattern<RtColorRGB>(sCtx->numPts);
		}
	}



	RtColorRGB *result = (RtColorRGB*) out[k_value].value;

	RtFloat3 const *P;
	RtFloat3 *Pw;
	RtVector3 temp(0,0,0);

	sCtx->GetParamInfo(k_position, &type, &cinfo);
	if (cinfo == k_RixSCNetworkValue)
	{
		sCtx->EvalParam(k_position, -1, &P, &temp, true);
		Pw = (RtFloat3*)P;
	}
	else
	{
		// __Pref and Po are not defined for volumes
		if (sCtx->scTraits.volume != NULL)
		{
			sCtx->GetBuiltinVar(RixShadingContext::k_P, &P);
		}
		else
		{
			RixSCDetail pDetail = sCtx->GetPrimVar(RtUString("__Pref"), RtFloat3(0.0f), (const RtFloat3**)&Pw);
			if (pDetail == k_RixSCInvalidDetail)
			{
				sCtx->GetBuiltinVar(RixShadingContext::k_Po, &P);
				Pw = pool.AllocForPattern<RtPoint3>(sCtx->numPts);
				memcpy(Pw, P, sizeof(RtFloat3) * sCtx->numPts);
				sCtx->Transform(RixShadingContext::k_AsPoints, Rix::k_current, data->coordsys, Pw, NULL);
			}
		}
	}

	//TODO check GEO_VolumeSampler.h

	if (data->volume->getTypeId() == GEO_PRIMVOLUME)
	{
		for (int i = 0; i < sCtx->numPts; ++i)
		{
			float res;
			res = ((GEO_PrimVolume*)(data->volume))->getValue( ((UT_Vector3*)Pw)[i]);
			result[i] = RtColorRGB(res);
		}
	}
	else if(data->volume->getTypeId() == GEO_PRIMVDB)
	{
		GEO_PrimVDB* vdb = (GEO_PrimVDB*)(data->volume);
		int nchannels = vdb->getTupleSize();
		if (nchannels==3)
		{
			vdb->getValues((UT_Vector3*)result, 1, (UT_Vector3*)Pw, sCtx->numPts);
		}
		else if (nchannels==1)
		{
			vdb->getValues((float*)result, 3, (UT_Vector3*)Pw, sCtx->numPts);

			for (int i = 0; i < sCtx->numPts; ++i)
			{
				result[i].g = result[i].b = result[i].r;
			}
		}
	}

	return 0;
}


RIX_PATTERNCREATE
{
	PIXAR_ARGUSED(hint);

	return new sampleVolume();
}

RIX_PATTERNDESTROY
{
	delete static_cast<sampleVolume*>(pattern);
}
