Shader "StdPipeline/Skybox" {
	HLSL : "511b6c06-86c8-468d-b9b9-ec0280ad3036"
	RootSignature {
		SRV[1] : 0
		CBV : 0
	}
	Properties {
		gSkybox("skybox", Cube) : Black
	}
	Pass (VS, PS) {}
}
