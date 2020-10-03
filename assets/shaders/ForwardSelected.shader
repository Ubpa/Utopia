Shader "StdPipeline/Forward Selected" {
	HLSL : "108e424b-d638-4d70-95fa-a354415a77e0"
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
		gOutlineWidth("outline width", float) : 0.1
	}
	Pass (VS2, PS2) {
		Tags {
			"LightMode" : "Forward"
		}
		ZWriteOff
		Cull Front
	}
	Pass (VS, PS) {
		Tags {
			"LightMode" : "Forward"
		}
	}
}
