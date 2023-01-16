#include "RixPattern.h"
#include "RixPredefinedStrings.hpp"

#include "hGeoStructsRIS.h"

#include <GU/GU_Detail.h>
#include <GU/GU_RayIntersect.h>

#include <map>
#include <iostream>

class closest: public RixPattern
{
public:
	enum paramId
	{
		// outputs
		CLOSEST_DATA_IDS
		k_dist,

		// inputs
		k_filename,
		k_maxdist,
		k_coordsys,

		// end of list
		k_numParams
	};

	struct Data
	{
		RtUString coordsys;
		GU_RayIntersect *isect;
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
	std::unordered_map<RtUString, GU_RayIntersect*> m_isect;
};


int
closest::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	return 0;
}


void
closest::Finalize(RixContext &ctx)
{
	for (auto item: m_isect)
	{
		delete item.second->detail();
		delete item.second;
	}
	PIXAR_ARGUSED(ctx);
}


RixSCParamInfo const*
closest::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs
		CLOSEST_DATA_OUT("ClosestData")
		RixSCParamInfo(RtUString("dist"), k_RixSCFloat, k_RixSCOutput),

		// inputs
		RixSCParamInfo(RtUString("filename"), k_RixSCString),
		RixSCParamInfo(RtUString("maxdist"), k_RixSCFloat),
		RixSCParamInfo(RtUString("coordsys"), k_RixSCString),


		// end of table
		RixSCParamInfo()
	};
	return &s_ptable[0];
}


void closest::CreateInstanceData(RixContext& ctx,
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

	data->coordsys = Rix::k_object;
	params->EvalParam(k_coordsys, -1, &data->coordsys);
	if (data->coordsys.Empty())
		data->coordsys = Rix::k_object;


	data->isect = nullptr;

	if (!filename.Empty())
	{
		auto it = m_isect.find(filename);

		if (it == m_isect.end())
		{
			GU_Detail * gdp = new GU_Detail;
			if (gdp->load(filename.CStr()).success())
			{
				// TODO: group searh
				GA_PrimitiveGroup* grp = nullptr;

				// Flag 'picking' should be set to 1.  When set to 0, curves and surfaces will be polygonalized.
				GU_RayIntersect *isect = new GU_RayIntersect(gdp, grp, 1, 0, 1);
				m_isect[filename] = isect;
				data->isect = isect;
				// std::cout << "Loaded "<< gdp->getNumPoints() << " points: " << filename.CStr() << " " << isect->getMemoryUsage(true) << std::endl;
			}
		}
		else
		{
			data->isect = it->second;
		}
	}

	return;
}


int
closest::ComputeOutputParams(RixShadingContext const *sCtx,
									int *nOutputs,
									OutputSpec **outputs,
									RtPointer instanceData,
									RixSCParamInfo const *instanceTable)
{

	PIXAR_ARGUSED(instanceTable);

	Data const* data = static_cast<Data const*>(instanceData);

	if (!data->isect)
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

		if (type == k_RixSCStructEnd)
			continue;

		else if (type == k_RixSCStructBegin)
			continue;

		else if( type == k_RixSCInteger )
		{
			if (i == k_meshID){
				out[i].detail = k_RixSCUniform;
				out[i].value = pool.AllocForPattern<RtInt>(1);
			}
			else
			{
				out[i].detail = k_RixSCVarying;
				out[i].value = pool.AllocForPattern<RtInt>(sCtx->numPts);
			}
		}

		else if( type == k_RixSCFloat )
		{
			out[i].detail = k_RixSCVarying;
			out[i].value = pool.AllocForPattern<RtFloat>(sCtx->numPts);
		}
	}

	// as optimisation we pass pointer to loaded geometry to next reader
	out[k_meshID].value = (RtInt*)data->isect->detail();

	RtInt* prim = (RtInt*) out[k_prim].value;
	RtFloat *u = (RtFloat*) out[k_u].value;
	RtFloat *v = (RtFloat*) out[k_v].value;
	RtFloat *dist = (RtFloat*) out[k_dist].value;

	RtFloat3 const *P;
	RtFloat3 *Pw = pool.AllocForPattern<RtPoint3>(sCtx->numPts);

	RixSCDetail pDetail = sCtx->GetPrimVar(RtUString("__Pref"), RtFloat3(0.0f), &P);
	if (pDetail == k_RixSCInvalidDetail)
		sCtx->GetBuiltinVar(RixShadingContext::k_Po, &P);
	
	sCtx->GetBuiltinVar(RixShadingContext::k_P, &P);


	memcpy(Pw, P, sizeof(RtFloat3) * sCtx->numPts);

	sCtx->Transform(RixShadingContext::k_AsPoints, Rix::k_current, data->coordsys, Pw, NULL);
	
	UT_Vector3 pos;

	const float *maxd;
	RtFloat const dflt(1E18F);
	sCtx->EvalParam(k_maxdist, -1, &maxd, &dflt, true);


	GU_MinInfo min_info;
	GA_PrimitiveTypeId primtype;
	for (int i = 0; i < sCtx->numPts; ++i)
	{
		pos.x() = Pw[i].x;
		pos.y() = Pw[i].y;
		pos.z() = Pw[i].z;

		min_info.init(maxd[i]*maxd[i]);
		if (data->isect->minimumPoint( pos, min_info ))
		{
			primtype = min_info.prim->getTypeId();
			u[i] = min_info.u1;

			// Normalize Polyline parametrization
			if ((primtype==GA_PRIMPOLY) && !min_info.prim.isClosed())
				u[i] = min_info.u1/(min_info.prim->getVertexCount()-1);

			v[i] = min_info.v1;
			// Normalize Polyline parametrization
			if ((primtype==GA_PRIMPOLY) && !min_info.prim.isClosed() || (type==GA_PRIMNURBCURVE) || (type==GA_PRIMBEZCURVE))
				v[i] = min_info.prim->calcPerimeter();

			prim[i] = min_info.prim.offset();
			dist[i] = sqrt(min_info.d);

		}
		else
		{
			prim[i] = -1;
			u[i] = 0;
			v[i] = 0;
			dist[i] = maxd[i];
		}
	}

	return 0;
}


RIX_PATTERNCREATE
{
	PIXAR_ARGUSED(hint);

	return new closest();
}

RIX_PATTERNDESTROY
{
	delete static_cast<closest*>(pattern);
}
