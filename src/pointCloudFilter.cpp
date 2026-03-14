#include <RixPattern.h>
#include <RixPredefinedStrings.hpp>

#include <GU/GU_Detail.h>
#include <GU/GU_PrimPacked.h>
#include <GEO/GEO_PointTree.h>
#include <GOP/GOP_Manager.h>

#include <RixShading.h>
#include <unordered_map>

class pointCloudFilter: public RixPattern
{
public:
	enum paramId
	{
		// outputs
		k_value,

		// inputs
		k_position,
		k_filename,
		k_pointgroup,
		k_maxdist,
		k_numPoints,
		k_coordsys,
		k_frame,
		k_fps,
		k_attributeName,
		k_matchVex,
		k_smoothBorders,

		// end of list
		k_numParams
	};

	struct Data
	{
		RtUString coordsys;
		RtUString attributeName;
		GEO_PointTreeGAOffset *tree;
		GU_Detail *gdp;
		bool matchVex;
		bool smoothBorders;
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
	std::unordered_map<RtUString, std::pair<GU_Detail*,GEO_PointTreeGAOffset*>> m_trees;
	RixMessages *m_msg {nullptr};
};


int
pointCloudFilter::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	m_msg = (RixMessages*)ctx.GetRixInterface(k_RixMessages);
	if (!m_msg) return 1;

	return 0;
}


void
pointCloudFilter::Finalize(RixContext &ctx)
{
	PIXAR_ARGUSED(ctx);
	for (auto item: m_trees)
	{
		delete item.second.first; // gdp
		delete item.second.second; // tree
	}
}


RixSCParamInfo const*
pointCloudFilter::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs
		RixSCParamInfo(RtUString("Value"),		k_RixSCColor, k_RixSCOutput),

		// inputs
		RixSCParamInfo(RtUString("position"),	k_RixSCColor),
		RixSCParamInfo(RtUString("filename"),	k_RixSCString),
		RixSCParamInfo(RtUString("pointgroup"),	k_RixSCString),
		RixSCParamInfo(RtUString("maxdist"),	k_RixSCFloat),
		RixSCParamInfo(RtUString("numPoints"),	k_RixSCInteger),
		RixSCParamInfo(RtUString("coordsys"),	k_RixSCString),
		RixSCParamInfo(RtUString("frame"),		k_RixSCFloat),
		RixSCParamInfo(RtUString("fps"),		k_RixSCFloat),
		RixSCParamInfo(RtUString("attribute"),	k_RixSCString),
		RixSCParamInfo(RtUString("matchVex"),	k_RixSCInteger),
		RixSCParamInfo(RtUString("smoothBorders"),	k_RixSCInteger),

		// end of table
		RixSCParamInfo()
	};
	return &s_ptable[0];
}


void pointCloudFilter::CreateInstanceData(RixContext& ctx,
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

	RtUString pointgroup = US_NULL;
	params->EvalParam(k_pointgroup, -1, &pointgroup);

	data->coordsys = Rix::k_object;
	params->EvalParam(k_coordsys, -1, &data->coordsys);

	float frame = 0;
	float fps = 24;
	params->EvalParam(k_frame, -1, &frame);
	params->EvalParam(k_fps, -1, &fps);

	RtUString attribName("Cd");
	params->EvalParam(k_attributeName, -1, &attribName);
	data->attributeName = attribName;

	int matchVex = 0;
	params->EvalParam(k_matchVex, -1, &matchVex);
	data->matchVex = matchVex;

	int smoothBorders = 0;
	params->EvalParam(k_smoothBorders, -1, &smoothBorders);
	data->smoothBorders = smoothBorders;

	data->gdp = nullptr;
	data->tree = nullptr;

	if (filename.Empty() || attribName.Empty())
		return;

	if (!filename.Empty())
	{
		char buff[255];
		sprintf(buff, "%s:%s:%g", filename.CStr(), pointgroup.CStr(), frame*fps);
		RtUString key(buff);

		auto it = m_trees.find(key);

		if (it == m_trees.end())
		{
			GU_Detail *gdp = new GU_Detail;
			if (gdp->load(filename.CStr()).success())
			{

				// Attempt to unpack all Packeds, Alembics and USD
				while (GU_PrimPacked::hasPackedPrimitives(*gdp))
				{
					for (GA_Iterator it(gdp->getPrimitiveRange()); !it.atEnd(); ++it)
					{
						if(GU_PrimPacked::isPackedPrimitive(gdp->getPrimitive(*it)->getTypeDef()))
						{
							const GU_PrimPacked* packed = UTverify_cast<const GU_PrimPacked*>(gdp->getPrimitive(*it));
							gdp->getPrimitive(*it)->setIntrinsic(packed->findIntrinsic("usdFrame"), frame);
							gdp->getPrimitive(*it)->setIntrinsic(packed->findIntrinsic("abcframe"), frame/fps);

							GU_Detail dest;
							packed->unpackUsingPolygons(dest);

							// replace packed with poly version
							gdp->destroyPrimitive(*gdp->getPrimitive(*it), true); // assume it's safe to delete prim while iterating
							gdp->mergePrimitives(dest, dest.getPrimitiveRange());
						}
					}
				}

				GOP_Manager group_manager;
				const GA_PointGroup* grp = nullptr;
				if (!pointgroup.Empty())
				{
					grp = group_manager.parsePointGroups(pointgroup.CStr(), GOP_Manager::GroupCreator(gdp));
					if (!grp)
						return;
				}

				GEO_PointTreeGAOffset *tree = new GEO_PointTreeGAOffset;
				tree->build(gdp, grp);
				m_trees[key] = std::pair<GU_Detail*,GEO_PointTreeGAOffset*>(gdp, tree);
				data->gdp = gdp;
				data->tree = tree;

				float mem = gdp->getMemoryUsage(true) + tree->getMemoryUsage(true);
				int idx = 0;
				while(mem>=1024)
				{
					mem /= 1024.0;
					idx++;
				}
				constexpr const char FILE_SIZE_UNITS[4][3] {"B", "KB", "MB", "GB"};
				m_msg->Info("[hGeo::pointCloudFilter] Loaded: %d points from %s %.1f %s (%s)", tree->entries(), filename.CStr(), mem, FILE_SIZE_UNITS[idx], handle.CStr() );
			}
			else
				m_msg->Warning("[hGeo::pointCloudFilter] Can't read file: %s (%s)", filename.CStr(), handle.CStr() );
		}
		else
		{
			data->tree = it->second.second;
			data->gdp = it->second.first;
		}
	}

	return;
}

float clamp(float x, float lowerlimit = 0.0f, float upperlimit = 1.0f) {
  if (x < lowerlimit) return lowerlimit;
  if (x > upperlimit) return upperlimit;
  return x;
}

float smoothstep (float edge0, float edge1, float x) {
   // Scale, and clamp x to 0..1 range
   x = clamp((x - edge0) / (edge1 - edge0));

   return x * x * (3.0f - 2.0f * x);
}


int
pointCloudFilter::ComputeOutputParams(RixShadingContext const *sCtx,
									int *nOutputs,
									OutputSpec **outputs,
									RtPointer instanceData,
									RixSCParamInfo const *instanceTable)
{

	PIXAR_ARGUSED(instanceTable);

	Data const* data = static_cast<Data const*>(instanceData);

	if (!data->tree)
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
		RixSCDetail pDetail = sCtx->GetPrimVar(RtUString("__Pref"), RtFloat3(0.0f), (const RtFloat3**)&Pw);
		if (pDetail == k_RixSCInvalidDetail)
		{
			// __Pref and Po are not defined for volumes
			if (sCtx->scTraits.volume != NULL)
				sCtx->GetBuiltinVar(RixShadingContext::k_P, &P);
			else
				sCtx->GetBuiltinVar(RixShadingContext::k_Po, &P);

			Pw = pool.AllocForPattern<RtPoint3>(sCtx->numPts);
			memcpy(Pw, P, sizeof(RtFloat3) * sCtx->numPts);
			sCtx->Transform(RixShadingContext::k_AsPoints, Rix::k_current, data->coordsys, Pw, NULL);
		}
	}

	const float *maxDistance;
	RtFloat const maxDistanceDefault(1.0);
	sCtx->EvalParam(k_maxdist, -1, &maxDistance, &maxDistanceDefault, true);

	const int *numPoints;
	int const numPointsDefault(10);
	sCtx->EvalParam(k_numPoints, -1, &numPoints, &numPointsDefault, true);

	GEO_PointTree::IdxArrayType plist;
	UT_FloatArray distances;
	
	GA_ROHandleV3 handle_v(data->gdp, GA_ATTRIB_POINT, data->attributeName.CStr());
	GA_ROHandleF handle_f(data->gdp, GA_ATTRIB_POINT, data->attributeName.CStr());

	for (int i = 0; i < sCtx->numPts; ++i)
	{
		result[i] = {0,0,0};
		UT_Vector3 pos(Pw[i].x, Pw[i].y, Pw[i].z);

		int maxNumPoints = numPoints[i];

		if (!data->matchVex)
			maxNumPoints++;

		const int found = data->tree->findNearestGroupIdx(pos, maxDistance[i], maxNumPoints, plist, distances);
		if (!found)
			continue;

		if (handle_v.isValid())
		{
			float weights = 0;
			for (int n=0; n<found; n++)
			{
				float weight = 0;

				if (data->matchVex)
					weight = smoothstep(sqrt(distances[found-1]) * 1.1, 0.f, sqrt(distances[n]));
				else if (found==maxNumPoints)
					weight = smoothstep(sqrt(distances[found-1]), 0.f, sqrt(distances[n]));
				else
					weight = smoothstep(maxDistance[i], 0.f, sqrt(distances[n]));
				
				UT_Vector3 attr = handle_v.get(plist[n]);
				result[i] += RtColorRGB(attr.x(), attr.y(), attr.z()) * weight;

				weights += weight;
			}

			if (weights>0)
				result[i] /= weights;

			if (!data->matchVex && data->smoothBorders)
				result[i] *= smoothstep(0.0, 1.0, weights); 
		}
		else if (handle_f.isValid())
		{
			float weights = 0;
			for (int n=0; n<found; n++)
			{
				float weight = 0;

				if (data->matchVex)
					weight = smoothstep(sqrt(distances[found-1]) * 1.1, 0.f, sqrt(distances[n]));
				else if (found==maxNumPoints)
					weight = smoothstep(sqrt(distances[found-1]), 0.f, sqrt(distances[n]));
				else
					weight = smoothstep(maxDistance[i], 0.f, sqrt(distances[n]));
				
				float attr = handle_f.get(plist[n]);
				result[i] += RtColorRGB(attr) * weight;

				weights += weight;
			}

			if (weights>0)
				result[i] /= weights;

			if (!data->matchVex && data->smoothBorders)
				result[i] *= smoothstep(0.0, 1.0, weights); 
		}

	}

	return 0;
}


RIX_PATTERNCREATE
{
	PIXAR_ARGUSED(hint);

	return new pointCloudFilter();
}

RIX_PATTERNDESTROY
{
	delete static_cast<pointCloudFilter*>(pattern);
}
