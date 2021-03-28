Shader "StdPipeline/Geometry" {
	HLSL : "008cb2d7-59b2-45e1-a765-36538a9d12ec"
	RootSignature {
		SRV[1] : 0
		SRV[1] : 1
		SRV[1] : 2
		SRV[1] : 3
		SRV[1] : 4
		CBV : 0
		CBV : 1
		CBV : 2
	}
	Properties {
		gAlbedoMap   ("albedo"   , 2D) : White
		gEmissionMap ("metalness", 2D) : Black
		gMetalnessMap("metalness", 2D) : White
		gRoughnessMap("roughness", 2D) : White
		gNormalMap   ("albedo"   , 2D) : Bump

		gAlbedoFactor   ("albedo factor"  , Color3) : (1, 1, 1)
		gEmissionFactor ("emission factor", Color3) : (1, 1, 1)
		gRoughnessFactor("roughness factor", float) : 1
		gMetalnessFactor("metalness factor", float) : 1
	}
	Pass (VS, PS) {
		Tags {
			"LightMode" : "Deferred"
		}
	}
}
