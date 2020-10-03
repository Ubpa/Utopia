Shader "StdPipeline/Forward" {
	HLSL : "c08331a5-339c-4e87-a739-3d6585c5b999"
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
	Pass (VS, PS) {
		Tags {
			"LightMode" : "Forward"
		}
	}
}
