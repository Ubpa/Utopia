Shader "StdPipeline/Geometry" {
	HLSL : "d1ee38ec-7485-422e-93f3-c8886169a858"
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
