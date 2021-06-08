Shader "StdPipeline/Wireframe" {
	// ForwardSimpleColor.hlsl
	HLSL : "472dc709-ced9-439c-965f-7035c23ee326"
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
