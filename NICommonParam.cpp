#include "NICommonParam.h"

namespace DxOpenNI {

NICommonParam::NICommonParam()
{
	init();
}

NICommonParam::~NICommonParam()
{ }

tc_string NICommonParam::GetHistorySizeString() const
{
	tc_ostringstream o;
	o << m_historySize;
	return o.str();
}

bool NICommonParam::PlusHistorySizePow(int d)
{
	if (d == 0) { return false; }

	if (0 < d) {
		while (0 < d--) {
			if (MAX_HISTORY <= m_historySize) {
				m_historySize = MAX_HISTORY;
				break;
			}
			++m_historySizePow;
			m_historySize *= 2;
		}

	} else {
		while (d++ < 0) {
			if (m_historySize <= MIN_HISTORY) {
				m_historySize = MIN_HISTORY;
				break;
			}
			--m_historySizePow;
			m_historySize /= 2;
		}
	}
	return true;
}

bool NICommonParam::SetHistorySizePow(int v)
{
	if (v < MIN_HISTORY_POW) {
		v = MIN_HISTORY_POW;
	} else if (MAX_HISTORY_POW < v) {
		v = MAX_HISTORY_POW;
	}
	if (v == m_historySizePow) { return false; }

	int newsize = 1;
	for (int x=v; 0<x; --x) {
		newsize *= 2;
	}
	m_historySizePow = v;
	m_historySize = newsize;
	return true;
}

}
