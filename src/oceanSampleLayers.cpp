#include "RixPattern.h"
#include "RixPredefinedStrings.hpp"

#include <CVEX/CVEX_Context.h>

#include <thread>
#include <mutex>


static std::mutex mutex;

class oceanSampleLayers: public RixPattern
{
public:
	enum paramId
	{
		// outputs
		k_displacement,
		k_velocity,
		k_cusp,
		k_cuspdir,

		// inputs
		k_filename,
		k_maskname,
		k_time,
		k_samplepos,
		k_aablur,
		k_falloffmode,
		k_falloffscale,
		k_downsample,

		// end of list
		k_numParams
	};

	struct Data
	{
		static void Free(RtPointer dataPtr);
		CVEX_StringArray *filename;
		CVEX_StringArray *maskname;
		std::unordered_map<std::thread::id,CVEX_Context*> *contexts;
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
	// Shader Defaults
	const RtFloat	m_time			= 0;
	const RtPoint3	m_samplepos		= {0,0,0};
	const RtFloat	m_aablur		= 1;
	const RtInt		m_falloffmode	= 0;
	const RtFloat	m_falloffscale	= 1.0;
	const RtInt		m_downsample	= 0;

	const char* m_programmName = "oceanSampleLayers";
	RixMessages *m_msg {nullptr};


};


void
oceanSampleLayers::Data::Free(RtPointer dataPtr)
{
	Data* data = static_cast<Data*>(dataPtr);

	if (data->filename)
		delete data->filename;
	if (data->maskname)
		delete data->maskname;

	if (data->contexts)
	{
		for (auto item: *data->contexts){
			delete item.second;
		}
		delete(data->contexts);
	}

	free(data);
}


int
oceanSampleLayers::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	m_msg = (RixMessages*)ctx.GetRixInterface(k_RixMessages);
	if (!m_msg) return 1;

	// Compiled VEX code is cached
	CVEX_Context().clearAllFunctions();

	return 0;
}


void
oceanSampleLayers::Finalize(RixContext &ctx)
{
	PIXAR_ARGUSED(ctx);
}


RixSCParamInfo const*
oceanSampleLayers::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs
		RixSCParamInfo(RtUString("displacement"), 	k_RixSCVector, k_RixSCOutput),
		RixSCParamInfo(RtUString("velocity"), 		k_RixSCVector, k_RixSCOutput),
		RixSCParamInfo(RtUString("cusp"), 			k_RixSCFloat, k_RixSCOutput),
		RixSCParamInfo(RtUString("cuspdir"), 		k_RixSCVector, k_RixSCOutput),

		// inputs
		RixSCParamInfo(RtUString("filename"), 		k_RixSCString),
		RixSCParamInfo(RtUString("maskname"), 		k_RixSCString),
		RixSCParamInfo(RtUString("time"), 			k_RixSCFloat),
		RixSCParamInfo(RtUString("samplepos"), 		k_RixSCPoint),
		RixSCParamInfo(RtUString("aablur"), 		k_RixSCFloat),
		RixSCParamInfo(RtUString("falloffmode"), 	k_RixSCInteger),
		RixSCParamInfo(RtUString("falloffscale"), 	k_RixSCFloat),
		RixSCParamInfo(RtUString("downsample"), 	k_RixSCInteger),

		// end of table
		RixSCParamInfo()
	};
	return &s_ptable[0];
}


void oceanSampleLayers::CreateInstanceData(RixContext& ctx,
										RtUString const handle,
										RixParameterList const* params,
										InstanceData* instanceData)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(handle);

	instanceData->datalen = sizeof(Data);
	instanceData->data = malloc(instanceData->datalen);
	instanceData->freefunc = Data::Free;;
	Data *data = static_cast<Data*>(instanceData->data);

	data->contexts = nullptr;
	data->filename = nullptr;
	data->maskname = nullptr;

	RtUString filename("");
	RtUString maskname("");

	params->EvalParam(k_filename, -1, &filename);
	params->EvalParam(k_maskname, -1, &maskname);

	if (filename.Empty())
	{
		m_msg->Warning("[hGeo::oceanSampleLayers] Spectrum Filename is Empty (%s)", handle );
		return;
	}

	// m_msg->Info("[hGeo::oceanSampleLayers] Using Spectrum File %s (%s)", filename, handle );

	data->filename = new CVEX_StringArray{filename.CStr()};
	data->maskname = new CVEX_StringArray{maskname.CStr()};

	CVEX_Context cvex;

	// Test Loading
	if (cvex.load(1, &m_programmName))
		data->contexts = new std::unordered_map<std::thread::id,CVEX_Context*>;

	if (cvex.getVexErrors().isstring())
		m_msg->Error((const char *)cvex.getVexErrors());
	if (cvex.getVexWarnings().isstring())
		m_msg->Warning((const char *)cvex.getVexWarnings());

	return;
}


int
oceanSampleLayers::ComputeOutputParams(RixShadingContext const *sCtx,
									int *nOutputs,
									OutputSpec **outputs,
									RtPointer instanceData,
									RixSCParamInfo const *instanceTable)
{

	PIXAR_ARGUSED(instanceTable);

	Data *data = (Data*)instanceData;

	if (data->contexts == nullptr)
		return 1;

	auto contexts = data->contexts;
	CVEX_Context *cvex;

	std::thread::id id = std::this_thread::get_id();
	auto it = contexts->find(id);

	// Create and CVEX_Context per thread and reuse it
	if (it == contexts->end())
	{
			cvex = new CVEX_Context();

			cvex->addInput("filename", *data->filename);
			cvex->addInput("maskname", *data->maskname);

			cvex->addInput("time",			CVEX_TYPE_FLOAT,	true);
			cvex->addInput("samplepos",		CVEX_TYPE_VECTOR3,	true);
			cvex->addInput("aablur",		CVEX_TYPE_FLOAT,	true);
			cvex->addInput("falloffmode",	CVEX_TYPE_INTEGER,	true);
			cvex->addInput("falloffscale",	CVEX_TYPE_FLOAT,	true);
			cvex->addInput("downsample",	CVEX_TYPE_INTEGER,	true);

			if (cvex->load(1, &m_programmName))
			{
				mutex.lock();
				contexts->emplace(id, cvex);
				mutex.unlock();
			}
	}
	else
	{
		cvex = it->second;
	}


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

		if( type == k_RixSCVector )
		{
			out[i].detail = k_RixSCVarying;
			out[i].value = pool.AllocForPattern<RtVector3>(sCtx->numPts);
			memset((void*)out[i].value, 0, sizeof(RtVector3) * sCtx->numPts); // Temporary
		}
		if( type == k_RixSCFloat )
		{
			out[i].detail = k_RixSCVarying;
			out[i].value = pool.AllocForPattern<RtFloat>(sCtx->numPts);
			memset((void*)out[i].value, 0, sizeof(RtFloat) * sCtx->numPts); // Temporary
		}
	}

	RtVector3 *displacement	= (RtVector3*)out[k_displacement].value;
	RtVector3 *velocity		= (RtVector3*)out[k_velocity].value;
	RtFloat   *cusp			= (RtFloat*)out[k_cusp].value;
	RtVector3 *cuspdir		= (RtVector3*)out[k_cuspdir].value;

	const RtFloat	*time;
	const RtPoint3	*samplepos;
	const RtFloat	*aablur;
	const RtInt		*falloffmode;
	const RtFloat	*falloffscale;
	const RtInt		*downsample;

	sCtx->EvalParam(k_time, 		-1, &time,			&m_time,			true);
	sCtx->EvalParam(k_samplepos, 	-1, &samplepos,		&m_samplepos,		true);
	sCtx->EvalParam(k_aablur, 		-1, &aablur,		&m_aablur,			true);
	sCtx->EvalParam(k_falloffmode, 	-1, &falloffmode,	&m_falloffmode,		true);
	sCtx->EvalParam(k_falloffscale, -1, &falloffscale,	&m_falloffscale,	true);
	sCtx->EvalParam(k_downsample, 	-1, &downsample,	&m_downsample,		true);


	CVEX_Value *time_val		 = cvex->findInput("time",			CVEX_TYPE_FLOAT);
	CVEX_Value *samplepos_val	 = cvex->findInput("samplepos",		CVEX_TYPE_VECTOR3);
	CVEX_Value *aablur_val		 = cvex->findInput("aablur",		CVEX_TYPE_FLOAT);
	CVEX_Value *falloffmode_val	 = cvex->findInput("falloffmode",	CVEX_TYPE_INTEGER);
	CVEX_Value *falloffscale_val = cvex->findInput("falloffscale",	CVEX_TYPE_FLOAT);
	CVEX_Value *downsample_val	 = cvex->findInput("downsample",	CVEX_TYPE_INTEGER);

	CVEX_Value *displacement_val = cvex->findOutput("displacement",	CVEX_TYPE_VECTOR3);
	CVEX_Value *velocity_val 	 = cvex->findOutput("velocity",		CVEX_TYPE_VECTOR3);
	CVEX_Value *cusp_val 		 = cvex->findOutput("cusp",			CVEX_TYPE_FLOAT);
	CVEX_Value *cuspdir_val 	 = cvex->findOutput("cuspdir",		CVEX_TYPE_VECTOR3);


	time_val->setTypedData(			(float*)time,				sCtx->numPts);
	samplepos_val->setTypedData(	(UT_Vector3*)samplepos,		sCtx->numPts);
	aablur_val->setTypedData(		(float*)aablur,				sCtx->numPts);
	falloffmode_val->setTypedData(	(int*)falloffmode,			sCtx->numPts);
	falloffscale_val->setTypedData(	(float*)falloffscale,		sCtx->numPts);
	downsample_val->setTypedData(	(int*)downsample,			sCtx->numPts);

	displacement_val->setTypedData(	(UT_Vector3*)displacement,	sCtx->numPts);
	velocity_val->setTypedData(		(UT_Vector3*)velocity,		sCtx->numPts);
	cusp_val->setTypedData(			(float*)cusp,				sCtx->numPts);
	cuspdir_val->setTypedData(		(UT_Vector3*)cuspdir,		sCtx->numPts);

	cvex->run(sCtx->numPts, false);

	return 0;
}


RIX_PATTERNCREATE
{
	PIXAR_ARGUSED(hint);
	return new oceanSampleLayers();
}

RIX_PATTERNDESTROY
{
	delete static_cast<oceanSampleLayers*>(pattern);
}
