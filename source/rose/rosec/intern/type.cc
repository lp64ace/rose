#include "COM_type.h"

#define IS(desired, tp) (desired == sizeof(tp))

#define ANY_1(rank) NULL
#define ANY_2(rank) IS(rank, char		) ? Tp_Char		: ANY_1(rank)
#define ANY_3(rank) IS(rank, short		) ? Tp_Short	: ANY_2(rank)
#define ANY_4(rank) IS(rank, int		) ? Tp_Int		: ANY_3(rank)
#define ANY_5(rank) IS(rank, long		) ? Tp_Long		: ANY_4(rank)
#define ANY_6(rank) IS(rank, long long	) ? Tp_LLong	: ANY_5(rank)
#define ANY ANY_6

CType *Tp_Int8 = ANY(sizeof(int8_t));
CType *Tp_UInt8 = ANY(sizeof(uint8_t));
CType *Tp_Int16 = ANY(sizeof(int16_t));
CType *Tp_UInt16 = ANY(sizeof(uint16_t));
CType *Tp_Int32 = ANY(sizeof(int32_t));
CType *Tp_UInt32 = ANY(sizeof(uint32_t));
CType *Tp_Int64 = ANY(sizeof(int64_t));
CType *Tp_UInt64 = ANY(sizeof(uint64_t));
