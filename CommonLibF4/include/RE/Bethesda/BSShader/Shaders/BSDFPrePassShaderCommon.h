#pragma once

namespace RE
{
	struct BSDFPrePassShaderCommonFeatures
	{
		enum
		{
			ALPHA_TEST = 1 << 8,
			LOD_LANDSCAPE = 1 << 9,
			TREE_ANIM = 1 << 10,
			LOD_OBJECT_INSTANCED = 1 << 11,
			CHARACTER_LIGHT_MASK = 1 << 12,
			MODELSPACENORMALS = 1 << 13,
			GLOWMAP = 1 << 14,
			BLEND = 1 << 15,
			MENU_SCREEN = 1 << 16,
			HAIR = 1 << 17,
			SKIN_TINT = 1 << 18,
			TESSELLATE_DISP_HEIGHT = 1 << 19,
			TESSELLATE_DISP_NORMALS = 1 << 20,
			DISMEMBERMENT = 1 << 21,
			DISMEMBERMENT_MEATCUFF = 1 << 22,
			PIPBOY_SCREEN = 1 << 23,
			ADDITIONAL_ALPHA_MASK = 1 << 24,
			LAND_LOD_BLEND = 1 << 25,
			GRADIENT_REMAP = 1 << 26,
			INSTANCED = 1 << 27,
			COMBINED = 1 << 28,
			CLIP_VOLUME = 1 << 29,
			BONE_TINTING = 1 << 30,
			FACE = 1 << 31,
			MERGE_INSTANCED = 402653184,
			SPLINE = 8389632,
			SKEW_SPECULAR_ALPHA = 67584
		};
	};
}
