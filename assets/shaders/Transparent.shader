Shader "StdPipeline/Transparent 2-sided" {
	HLSL : "aa8070f9-5eb9-46a9-8d3f-14f560e47d6e"
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
		gNormalMap("albedo", 2D) : Bump
		gAlbedoFactor("albedo factor", Color3) : (1, 1, 1)
		gRoughnessFactor("roughness factor", float) : 1
		gMetalnessFactor("metalness factor", float) : 1
	}
	Pass (VS, PS) {
		Tags {
			"LightMode" : "Forward"
		}
		Blend
		Cull Back
		Queue Transparent
	}
	Pass (VS, PS) {
		Tags {
			"LightMode" : "Forward"
		}
		Blend
		Cull Front
		Queue Transparent
	}
}
