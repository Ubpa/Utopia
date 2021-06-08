Shader "StdPipeline/RTwithBRDFLUT" {
	HLSL : "283eeaba-4401-4d5f-ab18-c969d1368889"
	RootSignature {
		SRV[2] : 0 // gSn, gUn
		SRV[3] : 2 // gbuffers
		SRV[1] : 5 // depth
		SRV[1] : 6 // BRDFLUT
		CBV : 0    // camera
	}
	Pass (VS, PS) {}
}
