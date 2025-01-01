#include "RixPattern.h"
#include "RixPredefinedStrings.hpp"

#include "hGeoStructsRIS.h"

#include <GU/GU_Detail.h>

#include <map>

class readAttribute: public RixPattern
{
public:
	enum paramId
	{
		// outputs
		ARRAY_DATA_IDS(ValuesA)

		// inputs
		ARRAY_DATA_IDS(IdxA)
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
	RixMessages *m_msg {nullptr};
};


int
readAttribute::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	m_msg = (RixMessages*)ctx.GetRixInterface(k_RixMessages);
	if (!m_msg) return 1;

	return 0;
}


void
readAttribute::Finalize(RixContext &ctx)
{
	for (auto item: m_geo){
		delete item.second;
	}
	PIXAR_ARGUSED(ctx);
}


RixSCParamInfo const*
readAttribute::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs
		ARRAY_DATA_OUT("ValuesA")

		// inputs
		ARRAY_DATA_IN("IdxA")
		RixSCParamInfo(RtUString("filename"), k_RixSCString),
		RixSCParamInfo(RtUString("attribute"), k_RixSCString),

		// end of table
		RixSCParamInfo()
	};
	return &s_ptable[0];
}


void readAttribute::CreateInstanceData(RixContext& ctx,
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

			float mem = gdp->getMemoryUsage(true);
			int idx = 0;
			while(mem>=1024)
			{
				mem /= 1024.0;
				idx++;
			}
			constexpr const char FILE_SIZE_UNITS[4][3] {"B", "KB", "MB", "GB"};
			m_msg->Info("[hGeo::readAttribute] Loaded: %s %.1f %s (%s)", filename.CStr(), mem, FILE_SIZE_UNITS[idx], handle.CStr() );
		}
		else
		{
			m_msg->Warning("[hGeo::readAttribute] Can't read file: %s (%s)", filename.CStr(), handle.CStr() );
			return;
		}
	}
	else
	{
		data->gdp = it->second;
	}

	GA_Attribute *attrib = data->gdp->findPointAttribute(attribName.CStr());
	if (attrib){
		int storage =  attrib->getStorageClass();
		int size = attrib->getTupleSize();
		if ((storage==0 || storage==1) && (size==1 || size==3))
		{
			data->attribute = attrib;
		}
	}

	return;
}


int
readAttribute::ComputeOutputParams(RixShadingContext const *sCtx,
									int *nOutputs,
									OutputSpec **outputs,
									RtPointer instanceData,
									RixSCParamInfo const *instanceTable)
{

	PIXAR_ARGUSED(instanceTable);

	RtInt const *mesh_id = nullptr;
	sCtx->EvalParam(k_IdxA_meshID, -1, &mesh_id);
	if (!mesh_id)
		return 1;

	Data const* data = static_cast<Data const*>(instanceData);

	GU_Detail *gdp;
	GA_Attribute *attribute = nullptr;

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
			GA_Attribute *attrib = gdp->findPointAttribute(data->attributeName.CStr());
			if (attrib){
				int storage =  attrib->getStorageClass();
				int size = attrib->getTupleSize();
				if ((storage==0 || storage==1) && (size==1 || size==3))
				{
					attribute = attrib;
				}
			}
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


	int const *numValues;
	sCtx->EvalParam(k_IdxA_num, -1, &numValues);

	if (!numValues)
		return 1;
	// looping through the different output ids
	int variable_num = 0;
	for (int i = 0; i < numOutputs; ++i)
	{
		out[i].paramId = i;
		out[i].detail = k_RixSCInvalidDetail;
		out[i].value = NULL;

		type = paramTable[i].type; // we know this

		sCtx->GetParamInfo(i, &type, &cinfo);

		if (type == k_RixSCStructEnd)
			continue;

		if (type == k_RixSCStructBegin)
		{
			variable_num = 0;
		}

		else if( type == k_RixSCInteger )
		{
			out[i].detail = k_RixSCUniform;
			out[i].value = pool.AllocForPattern<RtInt>(1);
		}

		else if( type == k_RixSCFloat3 )
		{
			if (variable_num < *numValues){
				out[i].detail = k_RixSCVarying;
				out[i].value = pool.AllocForPattern<RtFloat3>(sCtx->numPts);
			}
			variable_num++;
		}
	}

	// as optimisation we pass pointer to loaded geometry to next reader
	out[k_ValuesA_meshID].value = (RtInt*)gdp;

	sCtx->GetParamInfo(k_IdxA_ArrayData, &type, &cinfo);
	if (cinfo != k_RixSCNetworkValue) return 1;

	out[k_ValuesA_num].value = numValues;

	RtFloat3 const *idxs[8];

	RtFloat3 *resultV[8];
	for (int n=0; n<*numValues; n++)
	{
		sCtx->EvalParam(k_IdxA_v0+n, -1, &idxs[n]);
		resultV[n] = (RtFloat3*) out[k_ValuesA_v0+n].value;
	}


	int maxIdx = gdp->getNumPoints()-1;
	UT_Vector3 attr;

	GA_ROHandleV3 handle_v(gdp, GA_ATTRIB_POINT, data->attributeName.CStr());
	if (handle_v.isValid()){
		for (int i = 0; i < sCtx->numPts; ++i)
		{
			for (int n=0; n<*numValues; n++)
			{
				attr = handle_v.get( std::min(maxIdx, std::max(0, int(idxs[n][i].x))));
				resultV[n][i] = RtFloat3(attr.x(), attr.y(), attr.z());
			}
		}
		return 0;
	}

	GA_ROHandleF handle_f(gdp, GA_ATTRIB_POINT, data->attributeName.CStr());
	if (handle_f.isValid()){
		for (int i = 0; i < sCtx->numPts; ++i)
		{
			for (int n=0; n<*numValues; n++)
			{
				attr = handle_f.get( std::min(maxIdx, std::max(0, int(idxs[n][i].x))));
				resultV[n][i] = RtFloat3(attr.x(), attr.y(), attr.z());
			}
		}
		return 0;
	}
	return 1;

	GA_ROHandleI handle_i(gdp, GA_ATTRIB_POINT, data->attributeName.CStr());
	if (handle_i.isValid()){
		for (int i = 0; i < sCtx->numPts; ++i)
		{
			for (int n=0; n<*numValues; n++)
			{
				attr = handle_i.get( std::min(maxIdx, std::max(0, int(idxs[n][i].x))));
				resultV[n][i] = RtFloat3(attr.x(), attr.y(), attr.z());
			}
		}
		return 0;
	}
	return 0;
}


RIX_PATTERNCREATE
{
	PIXAR_ARGUSED(hint);

	return new readAttribute();
}

RIX_PATTERNDESTROY
{
	delete static_cast<readAttribute*>(pattern);
}
