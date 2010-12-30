#ifndef MMD_DXOPENNI_NICOMMONCONFIG_H
#define MMD_DXOPENNI_NICOMMONCONFIG_H

#include "tc.h"

namespace DxOpenNI {

// configuration which apply to all users
class NICommonParam
{
	NICommonParam(const NICommonParam&);
	NICommonParam& operator=(const NICommonParam&);
public:
	NICommonParam();
	~NICommonParam();

	inline void init();
	inline int GetHistorySize() const { return m_historySize; }

	enum {
		MIN_HISTORY = 1,
		MAX_HISTORY = 256,//128,
		MIN_HISTORY_POW = 0,
		MAX_HISTORY_POW = 7,
	};

	bool SetHistorySizePow(int d);
	bool PlusHistorySizePow(int d);
	tc_string GetHistorySizeString() const;
	inline int GetHistorySizePow() const { return m_historySizePow; }

private:
	int m_historySize;
	int m_historySizePow;
};

inline void NICommonParam::init()
{
	m_historySize = 1;
	m_historySizePow = 0;
}

}

#endif

