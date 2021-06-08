Shader "StdPipeline/TAA" {
	HLSL : "0fb7b700-f51c-4a07-84df-64398169b74c"
	RootSignature {
		SRV[2] : 0 // gPrevHDRImg, gCurrHDRImg
		SRV[1] : 2 // gMotion
		CBV : 0
	}
	Pass (VS, PS) {}
}
