Shader "StdPipeline/Forward Hole" {
	HLSL : "ea227832-c051-4304-86f4-dbf9454d5891"
	RootSignature {
		SRV[1] : 0
		SRV[1] : 1
		SRV[1] : 2
		SRV[1] : 3
		SRV[3] : 4
		CBV : 0
		CBV : 1
		CBV : 2
		CBV : 3
	}
	Properties {
		gAlbedoMap("albedo", 2D) : White
		gMetalnessMap("metalness", 2D) : White
		gRoughnessMap("roughness", 2D) : White
		gNormalMap("normal map", 2D) : Bump
		gAlbedoFactor("albedo factor", Color3) : (1, 1, 1)
		gRoughnessFactor("roughness factor", float) : 1
		gMetalnessFactor("metalness factor", float) : 1
	}
	Pass (HoleVS, HolePS) {
		Tags {
			"LightMode" : "Forward"
		}
		ZWriteOff
		ColorMask 0
		Stencil {
			Ref 1
			Pass Replace
		}
	}
	Pass (VS, PS) {
		Tags {
			"LightMode" : "Forward"
		}
		Stencil {
			Ref 1
			Comp NotEqual
		}
	}
}
