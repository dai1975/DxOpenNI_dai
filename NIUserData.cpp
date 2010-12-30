#include "NIUserData.h"
#include "DxOpenNI.h"

namespace DxOpenNI {

NIUserData::NIUserData()
{ }

NIUserData::~NIUserData()
{ }

bool NIUserData::Setup()
{
	m_bind = false;
	m_ready = 4;
	return true;
}

void NIUserData::Clean()
{
	m_bind = false;
	m_ready = 4;
}

bool NIUserData::Bind(XnUserID xnId, NICommonParam* pCommon)
{
	if (m_bind) { return false; }

	m_xnId = xnId;
	m_commonParam = pCommon;

	m_bind = true;
	m_ready = 4;

	m_historyLatest = 0;
	ClearCache();

	return true;
}

void NIUserData::Release()
{
	m_bind = false;
}

namespace {

// convert MMD(OpenNI)'s joint index to OpenNI's one.
// ex) the index of TORSO is zero in MMD but 3 in OpenNI,
//       DxOpenNI::JOINT_CENTER -> 0
//       MMD2XN[ DxOpenNI::JOINT_CENTER ] -> 3
XnSkeletonJoint MMD2XN[] = {
	XN_SKEL_TORSO,
	XN_SKEL_NECK,
	XN_SKEL_HEAD,
	XN_SKEL_LEFT_SHOULDER,
	XN_SKEL_LEFT_ELBOW,
	XN_SKEL_LEFT_HAND,
	XN_SKEL_RIGHT_SHOULDER,
	XN_SKEL_RIGHT_ELBOW,
	XN_SKEL_RIGHT_HAND,
	XN_SKEL_LEFT_HIP,
	XN_SKEL_LEFT_KNEE,
	XN_SKEL_LEFT_FOOT,
	XN_SKEL_RIGHT_HIP,
	XN_SKEL_RIGHT_KNEE,
	XN_SKEL_RIGHT_FOOT,
};

inline void _GetAllJoint
(xn::UserGenerator& gen,
 XnUserID xnId,
 const XnSkeletonJointPosition& zero,
 NIUserData::Snap& snap)
{
	for (int i=0; i<NUM_JOINT; ++i) {
		if (i == JOINT_CENTER) { continue; }

		XnSkeletonJoint xnJointId = MMD2XN[i];
		XnSkeletonJointPosition tmp;
		gen.GetSkeletonCap().GetSkeletonJointPosition(xnId, xnJointId, tmp);

		snap.joint[i].confidence = tmp.fConfidence;
		snap.joint[i].pos.x = (tmp.position.X - zero.position.X);
		snap.joint[i].pos.y = (tmp.position.Y - zero.position.Y);
		snap.joint[i].pos.z = (tmp.position.Z - zero.position.Z);
	}
	{
		NIUserData::JointData& l = snap.joint[JOINT_LEG_L];
		NIUserData::JointData& r = snap.joint[JOINT_LEG_R];
		snap.joint[JOINT_CENTER].pos = (l.pos + r.pos) / 2.0f;
		snap.joint[JOINT_CENTER].confidence = 
			(l.confidence < r.confidence)? l.confidence: r.confidence;
	} 
}

}

void NIUserData::Update(xn::UserGenerator& gen)
{
	if (0 < m_ready) {
		if (--m_ready == 0) {
			XnSkeletonJointPosition l,r;
			gen.GetSkeletonCap().GetSkeletonJointPosition(m_xnId, MMD2XN[JOINT_LEG_L], l);
			gen.GetSkeletonCap().GetSkeletonJointPosition(m_xnId, MMD2XN[JOINT_LEG_R], r);

			m_zero.position.X = (l.position.X + r.position.X) / 2.0f;
			m_zero.position.Y = (l.position.Y + r.position.Y) / 2.0f;
			m_zero.position.Z = (l.position.Z + r.position.Z) / 2.0f;

			_GetAllJoint(gen, m_xnId, m_zero, m_history[0]);
			for (int i=1; i<NICommonParam::MAX_HISTORY; ++i) {
				m_history[i] = m_history[0];
			}
			m_historyLatest = 0;
		}
		return;
	}

	int nextIndex = m_historyLatest + 1;
	if (nextIndex == NICommonParam::MAX_HISTORY) { nextIndex = 0; }

	_GetAllJoint(gen, m_xnId, m_zero, m_history[ nextIndex ]);
	m_historyLatest = nextIndex;
	ClearCache();
}

namespace {

inline bool _IsNear(const D3DXVECTOR3& target, const D3DXVECTOR3& last)
{
	float threthold = 10.0f;
	float f;
	f = target.x - last.x;
	if (f < 0) { f = -f; }
	if (threthold < f) { return false; }

	f = target.y - last.y;
	if (f < 0) { f = -f; }
	if (threthold < f) { return false; }

	f = target.z - last.z;
	if (f < 0) { f = -f; }
	if (threthold < f) { return false; }

	return true;
}

inline int _Sum(NIUserData::JointData* result, const NIUserData::JointData* current, const NIUserData::JointData* last)
{
	if (current->confidence < 0.5f) { return 0; }
//	if (! _IsNear(current.pos, last.pos)) { return 0; }

	result->pos += current->pos;
	last = current;
	return 1;
}

}

void NIUserData::CalcCache(JOINT_INDEX joint)
{
	if (joint < 0 || NUM_JOINT <= joint) { return; }

	int oldest = (m_historyLatest - m_commonParam->GetHistorySize() + 1);
	if (oldest < 0) {
		oldest += NICommonParam::MAX_HISTORY; // a[-1] -> a[a.size()-1]
	}

	JointData& result = m_cache.joint[joint];
	result.pos.x = result.pos.y = result.pos.z = 0;
	result.confidence = 1.0f;

	const JointData* pLast = &m_history[ oldest ].joint[joint];
	int n = 0;
	if (oldest <= m_historyLatest) {
		for (int i = oldest; i <= m_historyLatest; ++i) {
			n += _Sum(&result, &m_history[i].joint[joint], pLast);
		}
	} else {
		for (size_t i = oldest; i < NICommonParam::MAX_HISTORY; ++i) {
			n += _Sum(&result, &m_history[i].joint[joint], pLast);
		}
		for (int i = 0; i <= m_historyLatest; ++i) {
			n += _Sum(&result, &m_history[i].joint[joint], pLast);
		}
	}
	if (n == 0) {
		result.pos = m_history[ m_historyLatest ].joint[joint].pos;
	} else if (1 < n) {
		result.pos.x /= n;
		result.pos.y /= n;
		result.pos.z /= n;
	}
}

}


