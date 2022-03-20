Shader "StdPipeline/Directional Shadow" {
	HLSL : "aac4b785-5679-4180-9fd4-3321ce51fba8"
	RootSignature {
		CBV : 0
		CBV : 1
	}
	Pass (VS, PS) {
		Tags {
			"LightMode" : "DirectionalShadow"
		}
	}
}
