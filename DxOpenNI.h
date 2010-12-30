#ifndef MMD_DXOPENNI_H
#define MMD_DXOPENNI_H

namespace DxOpenNI {

enum JOINT_INDEX
{
	// use JointIndex same as to MMD
	//   0:center 1:neck 2:head 3:shoulderL 4:elbowL 5:handL
	//   6:shoulderR 7:elbowR 8:handR 9:legL 10:kneeL 11 ancleL
	//   12:legR 13:kneeR 14:ancleR
	JOINT_CENTER,
	JOINT_NECK,
	JOINT_HEAD,
	JOINT_SHOULDER_L,
	JOINT_ELBOW_L,
	JOINT_HAND_L,
	JOINT_SHOULDER_R,
	JOINT_ELBOW_R,
	JOINT_HAND_R,
	JOINT_LEG_L,
	JOINT_KNEE_L,
	JOINT_ANCLE_L,
	JOINT_LEG_R,
	JOINT_KNEE_R,
	JOINT_ANCLE_R,
	NUM_JOINT,
};

}

#endif

