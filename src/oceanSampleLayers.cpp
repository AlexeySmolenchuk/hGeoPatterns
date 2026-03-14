#include <RixPattern.h>
#include <RixInterfaces.h>
#include <RixPredefinedStrings.hpp>

#include <CVEX/CVEX_Context.h>
#include <CVEX/CVEX_Value.h>

#include <thread>
#include <mutex>
#include <unordered_map>


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

	struct ThreadData
	{
		CVEX_StringArray filename;
		CVEX_StringArray maskname;
		CVEX_Context cvex;
	};

	struct Data
	{
		static void Free(RtPointer dataPtr);
		std::mutex myMutex;
		std::unordered_map<std::thread::id,ThreadData> threadData;
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

	const char* m_programName = "oceanSampleLayers";
	RixMessages *m_msg {nullptr};
};


void
oceanSampleLayers::Data::Free(RtPointer dataPtr)
{
	delete static_cast<Data*>(dataPtr);
}


int
oceanSampleLayers::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	m_msg = (RixMessages*)ctx.GetRixInterface(k_RixMessages);
	if (!m_msg) return 1;

#ifndef NDEBUG
	// Compiled VEX code is cached. We can use this function for fresh reload.
	CVEX_Context().clearAllFunctions();
#endif
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
		RixSCParamInfo(RtUString("cusp"), 			k_RixSCFloat,  k_RixSCOutput),
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

	instanceData->data = nullptr;

	// Test Loading
	CVEX_Context cvex;
	
	if (!cvex.load(1, &m_programName))
	{
		if (cvex.getVexErrors().isstring())
			m_msg->Error((const char *)cvex.getVexErrors());
		if (cvex.getVexWarnings().isstring())
			m_msg->Warning((const char *)cvex.getVexWarnings());
		// exit without creating Data
		return;
	}

	instanceData->datalen = sizeof(Data);
	instanceData->data = new Data();
	instanceData->freefunc = Data::Free;

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

	if (!data)
		return 1;

	CVEX_Context *cvex;

	std::thread::id id = std::this_thread::get_id();
	auto it = data->threadData.find(id);

	// We can execute CVEX_Context multiple times for different input data
	if (it == data->threadData.end())
	{
		data->myMutex.lock();
		auto &ptd = data->threadData[id];
		data->myMutex.unlock();

		const RtUString *filename;
		const RtUString *maskname;
		
		sCtx->EvalParam(k_filename, -1, &filename, &Rix::k_empty);
		sCtx->EvalParam(k_maskname, -1, &maskname, &Rix::k_empty);

		if (filename->Empty())
			m_msg->Warning("[hGeo::oceanSampleLayers] Spectrum Filename is Empty");
		else
			m_msg->Info("[hGeo::oceanSampleLayers] Using Spectrum File %s", *filename);

		ptd.filename.append(filename->CStr());
		ptd.maskname.append(maskname->CStr());
		
		cvex = &ptd.cvex;

		// Uniform
		cvex->addInput("filename", ptd.filename);
		cvex->addInput("maskname", ptd.maskname);
		// Varying
		cvex->addInput("time",			CVEX_TYPE_FLOAT,	true);
		cvex->addInput("samplepos",		CVEX_TYPE_VECTOR3,	true);
		cvex->addInput("aablur",		CVEX_TYPE_FLOAT,	true);
		cvex->addInput("falloffmode",	CVEX_TYPE_INTEGER,	true);
		cvex->addInput("falloffscale",	CVEX_TYPE_FLOAT,	true);
		cvex->addInput("downsample",	CVEX_TYPE_INTEGER,	true);

		cvex->load(1, &m_programName);
	}
	else
	{
		cvex = &it->second.cvex;
	}


	RixSCType type;
	RixSCConnectionInfo cinfo;
	RixSCParamInfo const* paramTable = GetParamTable();
	int numOutputs = -1;
	while (paramTable[++numOutputs].access == k_RixSCOutput) {}

	RixShadingContext::Allocator pool(sCtx);
	OutputSpec *out = *outputs = pool.AllocForPattern<OutputSpec>(numOutputs);
	*nOutputs = numOutputs;

	// looping through the different output ids and allocate memory (even non connected)
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
		}
		if( type == k_RixSCFloat )
		{
			out[i].detail = k_RixSCVarying;
			out[i].value = pool.AllocForPattern<RtFloat>(sCtx->numPts);
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

	// Map input parameters and promote to varying
	sCtx->EvalParam(k_time, 		-1, &time,			&m_time,			true);
	sCtx->EvalParam(k_samplepos, 	-1, &samplepos,		&m_samplepos,		true);
	sCtx->EvalParam(k_aablur, 		-1, &aablur,		&m_aablur,			true);
	sCtx->EvalParam(k_falloffmode, 	-1, &falloffmode,	&m_falloffmode,		true);
	sCtx->EvalParam(k_falloffscale, -1, &falloffscale,	&m_falloffscale,	true);
	sCtx->EvalParam(k_downsample, 	-1, &downsample,	&m_downsample,		true);


	// Find Inputs/Outputs in VEX program
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


	// Map RenderMan arrays to CVex context
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

	// Execute
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
