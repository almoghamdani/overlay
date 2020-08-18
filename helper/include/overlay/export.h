#ifndef OVERLAY_EXPORT_H
#define OVERLAY_EXPORT_H

#ifdef OVERLAY_HELPER_MAKEDLL
#define HELPER_EXPORT __declspec(dllexport)
#else
#define HELPER_EXPORT __declspec(dllimport)
#endif

#endif