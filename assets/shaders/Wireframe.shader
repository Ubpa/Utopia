Shader "StdPipeline/Wireframe" {
	// ForwardColor.hlsl
	HLSL : "e6e36fe7-6068-4e98-a32a-1632779a5084"
	RootSignature {
		CBV : 0
		CBV : 1
		CBV : 2
	}
	Properties {
		gColor("color", Color3) : (1, 1, 1)
	}
	Pass (VS, PS) {
		Tags {
			"LightMode" : "Forward"
		}
		Fill Wireframe
		Cull Off
	}
}
