Shader "StdPipeline/Color" {
	HLSL : "c8b789e2-872d-48c5-b03b-5398b699d3bc"
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
	}
}
