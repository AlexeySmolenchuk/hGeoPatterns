#include "RixPattern.h"
#include "RixPredefinedStrings.hpp"

#include "hGeoStructsRIS.h"

#include <GU/GU_Detail.h>

#include <map>
#include <iostream>

class interpolator: public RixPattern
{
public:
	enum paramId
	{
		// outputs
		k_value,
		

		// inputs
		CLOSEST_DATA_IDS
		k_filename,
		k_attributeName,

		// end of list
		k_numParams
	};

	struct Data
	{
		RtUString attributeName;
		GA_Attribute *attribute;
		GU_Detail *gdp;
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
	std::unordered_map<RtInt64, GA_Attribute*> m_attributes;
};


int
interpolator::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	return 0;
}


void
interpolator::Finalize(RixContext &ctx)
{
	for (auto item: m_geo){
		delete item.second;
	}
	PIXAR_ARGUSED(ctx);
}


RixSCParamInfo const*
interpolator::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs
		RixSCParamInfo(RtUString("Value"), k_RixSCColor, k_RixSCOutput),

		// inputs
		CLOSEST_DATA_IN("ClosestData")
		RixSCParamInfo(RtUString("filename"), k_RixSCString),
		RixSCParamInfo(RtUString("attribute"), k_RixSCString),

		// end of table
		RixSCParamInfo()
	};
	return &s_ptable[0];
}


void interpolator::CreateInstanceData(RixContext& ctx,
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

	RtUString attribName("P");
	params->EvalParam(k_attributeName, -1, &attribName);
	data->attributeName = attribName;

	data->gdp = nullptr;
	data->attribute = nullptr;

	if (filename.Empty())
		return;

	auto it = m_geo.find(filename);

	if (it == m_geo.end())
	{
		GU_Detail * gdp = new GU_Detail;
		if (gdp->load(filename.CStr()).success())
		{
			m_geo[filename] = gdp;
			data->gdp = gdp;
			std::cout << "Loaded: " << filename.CStr() << " " << gdp->getMemoryUsage(true) <<std::endl;
		}
		else
		{
			return;
		}
	}
	else
	{
		data->gdp = it->second;
	}

	GA_Attribute *attrib = nullptr;
	attrib = data->gdp->findVertexAttribute( attribName.CStr() );
	if (!attrib) attrib = data->gdp->findPointAttribute( attribName.CStr() );
	if (!attrib) attrib = data->gdp->findPrimitiveAttribute( attribName.CStr() );
	
	data->attribute = attrib;

	return;
}


int
interpolator::ComputeOutputParams(RixShadingContext const *sCtx,
									int *nOutputs,
									OutputSpec **outputs,
									RtPointer instanceData,
									RixSCParamInfo const *instanceTable)
{

	PIXAR_ARGUSED(instanceTable);

	RtInt const *mesh_id = nullptr;
	sCtx->EvalParam(k_meshID, -1, &mesh_id);
	if (!mesh_id)
		return 1;

	Data const* data = static_cast<Data const*>(instanceData);

	GU_Detail *gdp;
	GA_Attribute *attribute;

	if (data->attribute)
	{
		gdp = data->gdp;
		attribute = data->attribute;
	}
	else
	{
		// need to test this data passing
		gdp = (GU_Detail*)mesh_id;

		RtInt64 key((RtInt64)mesh_id + data->attributeName.Hash());

		// if needed attribute not in class members map
		auto it = m_attributes.find(key);
		if (it == m_attributes.end())
		{
			attribute = gdp->findVertexAttribute( data->attributeName.CStr() );
			if (!attribute) attribute = gdp->findPointAttribute( data->attributeName.CStr() );
			if (!attribute) attribute = gdp->findPrimitiveAttribute( data->attributeName.CStr() );
			m_attributes[key] = attribute;
		}
		else
		{
			attribute = it->second;
		}
	}


	if (!attribute)
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

	RtInt const *prim;
	sCtx->EvalParam(k_prim, -1, &prim);

	RtFloat const *u;
	sCtx->EvalParam(k_u, -1, &u);

	RtFloat const *v;
	sCtx->EvalParam(k_v, -1, &v);

	UT_Vector3 attr;
	auto owner = attribute->getOwner();

	if(owner==GA_ATTRIB_PRIMITIVE)
	{
		// try to read vector attribute
		GA_ROHandleV3 handle_v(attribute);
		if (handle_v.isValid())
		{
			for (int i = 0; i < sCtx->numPts; ++i)
			{
				attr = prim[i]<0 ? UT_Vector3(0.0) : handle_v.get(prim[i]);
				result[i] = RtColorRGB(attr.x(), attr.y(), attr.z());
			}
			return 0;
		}
		// try to read float attribute
		GA_ROHandleF handle_f(attribute);
		if (handle_f.isValid())
		{
			for (int i = 0; i < sCtx->numPts; ++i)
			{
				attr = prim[i]<0 ? 0.0 : handle_f.get(prim[i]);
				result[i] = RtColorRGB(attr.x(), attr.y(), attr.z());
			}
			return 0;
		}
		// try to read int attribute
		GA_ROHandleI handle_i(attribute);
		if (handle_i.isValid())
		{
			for (int i = 0; i < sCtx->numPts; ++i)
			{
				attr = prim[i]<0 ? 0 : handle_i.get(prim[i]);
				result[i] = RtColorRGB(attr.x(), attr.y(), attr.z());
			}
			return 0;
		}
	}

	// Point and Vertex Attributes
	if((owner==GA_ATTRIB_POINT)||(owner==GA_ATTRIB_VERTEX))
	{
		UT_Array<GA_Offset> offsetarray;
		UT_FloatArray weightarray;
		GEO_Primitive *primitive;
		GA_Offset offset;

		// try to read vector attribute
		GA_ROHandleV3 handle_v(attribute);
		if (handle_v.isValid())
		{
			for (int i = 0; i < sCtx->numPts; ++i)
			{
				if (prim[i]<0)
					result[i] = RtColorRGB(0.0);
				else
				{
					primitive = gdp->getGEOPrimitive(prim[i]);
					primitive->computeInteriorPointWeights(offsetarray, weightarray, u[i], v[i], 0.0);

					attr = 0;

					for (int n = 0; n < offsetarray.size(); ++n)
					{
						offset = offsetarray(n);
						if (owner == GA_ATTRIB_POINT)
							offset =  gdp->vertexPoint(offset);
						attr += weightarray(n) * handle_v.get(offset);
					}
					result[i] = RtColorRGB(attr.x(), attr.y(), attr.z());
				}
			}
			return 0;
		}

		// try to read float attribute
		GA_ROHandleF handle_f(attribute);
		if (handle_f.isValid())
		{
			for (int i = 0; i < sCtx->numPts; ++i)
			{
				if (prim[i]<0)
					result[i] = RtColorRGB(0.0);
				else
				{
					primitive = gdp->getGEOPrimitive(prim[i]);
					primitive->computeInteriorPointWeights(offsetarray, weightarray, u[i], v[i], 0.0);

					attr = 0;

					for (int n = 0; n < offsetarray.size(); ++n)
					{
						offset = offsetarray(n);
						if (owner == GA_ATTRIB_POINT)
							offset =  gdp->vertexPoint(offset);
						attr += weightarray(n) * handle_f.get(offset);
					}
					result[i] = RtColorRGB(attr.x(), attr.y(), attr.z());
				}
			}
			return 0;
		}
	}

	return 1;
}


RIX_PATTERNCREATE
{
	PIXAR_ARGUSED(hint);

	return new interpolator();
}

RIX_PATTERNDESTROY
{
	delete static_cast<interpolator*>(pattern);
}
