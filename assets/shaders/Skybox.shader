Shader "StdPipeline/Skybox" {
	HLSL : "bdd78e15-6ec2-40fe-b7ce-93ce142d19a7"
	RootSignature {
		SRV[1] : 0
		CBV : 0
	}
	Pass (VS, PS) {
		Properties {
			gSkybox("skybox", Cube) : Black
		}
	}
}
