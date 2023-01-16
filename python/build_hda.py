
import sys
from shutil import move

# add custom optional parameter to args parser
from rman_utils.node_desc_param import NodeDescParam
NodeDescParam.optional_attrs.append("struct_name")

import rfh.convertutils

otlfile = sys.argv[1]
rixplugins = sys.argv[2]
shaders = sys.argv[3]

tmp = otlfile + ".tmp"

rfh.convertutils.main(  [rixplugins],
                        otlfile = tmp,
                        originalCase=True,
                        namespace="hGeo",
                        menu="hGeoPatterns",
                        version="1.0",
                        icon="PLASMA_App"
                        )

rfh.convertutils.main(  [shaders],
                        otlfile = tmp,
                        originalCase=True,
                        namespace="hGeo",
                        menu="hGeoPatterns",
                        version="1.0",
                        icon="VOP_oslbuilder"
                        )

move(tmp, otlfile)
