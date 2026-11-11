/**
 * @file Guid.h
 * @author LinhengXilan
 * @version 0.0.0.6
 * @date 2026-11-11
 */

#ifndef GUID_H
#define GUID_H

 // {60195181-4D07-4DF3-98BA-D58DA5E306C2}
namespace Guid
{
	constexpr GUID CREATOR_ID{ .Data1 = 0x60195181, .Data2 = 0x4d07, .Data3 = 0x4df3, .Data4 = { 0x98, 0xba, 0xd5, 0x8d, 0xa5, 0xe3, 0x6, 0xc2 }};
}

#endif