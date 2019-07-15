"#ifndef _SHADOWS_FXH_\n"
"#define _SHADOWS_FXH_\n"
"\n"
"// Must include BasicStructures.fxh\n"
"\n"
"#ifndef SHADOW_FILTER_SIZE\n"
"#   define SHADOW_FILTER_SIZE 2\n"
"#endif\n"
"\n"
"\n"
"void FindCascade(ShadowMapAttribs ShadowAttribs,\n"
"                 float3           f3PosInLightViewSpace,\n"
"                 float            fCameraViewSpaceZ,\n"
"                 out float3       f3PosInCascadeProjSpace,\n"
"                 out float3       f3CascadeLightSpaceScale,\n"
"                 out int          Cascade)\n"
"{\n"
"    f3PosInCascadeProjSpace  = float3(0.0, 0.0, 0.0);\n"
"    f3CascadeLightSpaceScale = float3(0.0, 0.0, 0.0);\n"
"    Cascade = 0;\n"
"#if BEST_CASCADE_SEARCH\n"
"    while (Cascade < ShadowAttribs.iNumCascades)\n"
"    {\n"
"        // Find the smallest cascade which covers current point\n"
"        CascadeAttribs CascadeAttribs = ShadowAttribs.Cascades[Cascade];\n"
"        f3CascadeLightSpaceScale = CascadeAttribs.f4LightSpaceScale.xyz;\n"
"        f3PosInCascadeProjSpace = f3PosInLightViewSpace * f3CascadeLightSpaceScale + ShadowAttribs.Cascades[Cascade].f4LightSpaceScaledBias.xyz;\n"
"        \n"
"        // In order to perform PCF filtering without getting out of the cascade shadow map,\n"
"        // we need to be far enough from its boundaries.\n"
"        if( //Cascade == (ShadowAttribs.iNumCascades - 1) || \n"
"            abs(f3PosInCascadeProjSpace.x) < 1.0 - CascadeAttribs.f4MaxFilterRadiusProjSpace.x &&\n"
"            abs(f3PosInCascadeProjSpace.y) < 1.0 - CascadeAttribs.f4MaxFilterRadiusProjSpace.y &&\n"
"            // It is necessary to check f3PosInCascadeProjSpace.z as well since it could be behind\n"
"            // the far clipping plane of the current cascade\n"
"            f3PosInCascadeProjSpace.z > NDC_MIN_Z + CascadeAttribs.f4MaxFilterRadiusProjSpace.z &&\n"
"            f3PosInCascadeProjSpace.z < 1.0       - CascadeAttribs.f4MaxFilterRadiusProjSpace.w )\n"
"            break;\n"
"        else\n"
"            Cascade++;\n"
"    }\n"
"#else\n"
"    [unroll]\n"
"    for(int i=0; i< (ShadowAttribs.iNumCascades+3)/4; ++i)\n"
"    {\n"
"        float4 f4CascadeZEnd = ShadowAttribs.f4CascadeCamSpaceZEnd[i];\n"
"        float4 v = float4( f4CascadeZEnd.x < fCameraViewSpaceZ ? 1.0 : 0.0, \n"
"                           f4CascadeZEnd.y < fCameraViewSpaceZ ? 1.0 : 0.0,\n"
"                           f4CascadeZEnd.z < fCameraViewSpaceZ ? 1.0 : 0.0,\n"
"                           f4CascadeZEnd.w < fCameraViewSpaceZ ? 1.0 : 0.0);\n"
"	    //float4 v = float4(ShadowAttribs.f4CascadeCamSpaceZEnd[i] < fCameraViewSpaceZ);\n"
"	    Cascade += int(dot(float4(1.0,1.0,1.0,1.0), v));\n"
"    }\n"
"    if( Cascade < ShadowAttribs.iNumCascades )\n"
"    {\n"
"    //Cascade = min(Cascade, ShadowAttribs.iNumCascades - 1);\n"
"        f3CascadeLightSpaceScale = ShadowAttribs.Cascades[Cascade].f4LightSpaceScale.xyz;\n"
"        f3PosInCascadeProjSpace = f3PosInLightViewSpace * f3CascadeLightSpaceScale + ShadowAttribs.Cascades[Cascade].f4LightSpaceScaledBias.xyz;\n"
"    }\n"
"#endif\n"
"}\n"
"\n"
"float2 ComputeReceiverPlaneDepthBias(float3 ShadowUVDepthDX,\n"
"                                     float3 ShadowUVDepthDY)\n"
"{    \n"
"    // Compute (dDepth/dU, dDepth/dV):\n"
"    //  \n"
"    //  | dDepth/dU |    | dX/dU    dX/dV |T  | dDepth/dX |     | dU/dX    dU/dY |-1T | dDepth/dX |\n"
"    //                 =                                     =                                      =\n"
"    //  | dDepth/dV |    | dY/dU    dY/dV |   | dDepth/dY |     | dV/dX    dV/dY |    | dDepth/dY |\n"
"    //\n"
"    //  | A B |-1   | D  -B |                      | A B |-1T   | D  -C |                                   \n"
"    //            =           / det                           =           / det                    \n"
"    //  | C D |     |-C   A |                      | C D |      |-B   A |\n"
"    //\n"
"    //  | dDepth/dU |           | dV/dY   -dV/dX |  | dDepth/dX |\n"
"    //                 = 1/det                                       \n"
"    //  | dDepth/dV |           |-dU/dY    dU/dX |  | dDepth/dY |\n"
"\n"
"    float2 biasUV;\n"
"    //               dV/dY       V      dDepth/dX    D       dV/dX       V     dDepth/dY     D\n"
"    biasUV.x =   ShadowUVDepthDY.y * ShadowUVDepthDX.z - ShadowUVDepthDX.y * ShadowUVDepthDY.z;\n"
"    //               dU/dY       U      dDepth/dX    D       dU/dX       U     dDepth/dY     D\n"
"    biasUV.y = - ShadowUVDepthDY.x * ShadowUVDepthDX.z + ShadowUVDepthDX.x * ShadowUVDepthDY.z;\n"
"\n"
"    float Det = (ShadowUVDepthDX.x * ShadowUVDepthDY.y) - (ShadowUVDepthDX.y * ShadowUVDepthDY.x);\n"
"	biasUV /= sign(Det) * max( abs(Det), 1e-10 );\n"
"    //biasUV = abs(Det) > 1e-7 ? biasUV / abs(Det) : 0;// sign(Det) * max( abs(Det), 1e-10 );\n"
"    return biasUV;\n"
"}\n"
"\n"
"\n"
"\n"
"\n"
"//-------------------------------------------------------------------------------------------------\n"
"// The method used in The Witness\n"
"//-------------------------------------------------------------------------------------------------\n"
"float FilterShadowMapOptimizedPCF(in Texture2DArray<float>  tex2DShadowMap,\n"
"                                  in SamplerComparisonState tex2DShadowMap_sampler,\n"
"                                  in float2                 shadowMapSize,\n"
"                                  in float3                 shadowPos,\n"
"                                  in int                    cascadeIdx,\n"
"                                  in float2                 receiverPlaneDepthBias)\n"
"{\n"
"    float lightDepth = shadowPos.z;\n"
"\n"
"    float2 uv = shadowPos.xy * shadowMapSize; // 1 unit - 1 texel\n"
"    float2 shadowMapSizeInv = 1.0 / shadowMapSize;\n"
"\n"
"    float2 base_uv = floor(uv + float2(0.5, 0.5));\n"
"    float s = (uv.x + 0.5 - base_uv.x);\n"
"    float t = (uv.y + 0.5 - base_uv.y);\n"
"\n"
"    base_uv -= float2(0.5, 0.5);\n"
"    base_uv *= shadowMapSizeInv;\n"
"\n"
"    float sum = 0;\n"
"\n"
"    // It is essential to clamp biased depth to 0 to avoid shadow leaks at near cascade depth boundary.\n"
"    //        \n"
"    //            No clamping                 With clamping\n"
"    //                                      \n"
"    //              \\ |                             ||    \n"
"    //       ==>     \\|                             ||\n"
"    //                |                             ||         \n"
"    // Light ==>      |\\                            |\\         \n"
"    //                | \\Receiver plane             | \\ Receiver plane\n"
"    //       ==>      |  \\                          |  \\   \n"
"    //                0   ...   1                   0   ...   1\n"
"    //\n"
"    // Note that clamping at far depth boundary makes no difference as 1 < 1 produces 0 and so does 1+x < 1\n"
"    const float DepthClamp = 1e-8;\n"
"#define SAMPLE_SHADOW_MAP(u, v) tex2DShadowMap.SampleCmp(tex2DShadowMap_sampler, float3(base_uv.xy + float2(u,v) * shadowMapSizeInv, cascadeIdx), max(lightDepth + dot(float2(u, v), receiverPlaneDepthBias), DepthClamp))\n"
"\n"
"    #if SHADOW_FILTER_SIZE == 2\n"
"\n"
"        return tex2DShadowMap.SampleCmp(tex2DShadowMap_sampler, float3(shadowPos.xy, cascadeIdx), max(lightDepth, DepthClamp));\n"
"\n"
"    #elif SHADOW_FILTER_SIZE == 3\n"
"\n"
"        float uw0 = (3.0 - 2.0 * s);\n"
"        float uw1 = (1.0 + 2.0 * s);\n"
"\n"
"        float u0 = (2.0 - s) / uw0 - 1.0;\n"
"        float u1 = s / uw1 + 1.0;\n"
"\n"
"        float vw0 = (3.0 - 2.0 * t);\n"
"        float vw1 = (1.0 + 2.0 * t);\n"
"\n"
"        float v0 = (2.0 - t) / vw0 - 1;\n"
"        float v1 = t / vw1 + 1;\n"
"\n"
"        sum += uw0 * vw0 * SAMPLE_SHADOW_MAP(u0, v0);\n"
"        sum += uw1 * vw0 * SAMPLE_SHADOW_MAP(u1, v0);\n"
"        sum += uw0 * vw1 * SAMPLE_SHADOW_MAP(u0, v1);\n"
"        sum += uw1 * vw1 * SAMPLE_SHADOW_MAP(u1, v1);\n"
"\n"
"        return sum * 1.0 / 16.0;\n"
"\n"
"    #elif SHADOW_FILTER_SIZE == 5\n"
"\n"
"        float uw0 = (4.0 - 3.0 * s);\n"
"        float uw1 = 7.0;\n"
"        float uw2 = (1.0 + 3.0 * s);\n"
"\n"
"        float u0 = (3.0 - 2.0 * s) / uw0 - 2.0;\n"
"        float u1 = (3.0 + s) / uw1;\n"
"        float u2 = s / uw2 + 2.0;\n"
"\n"
"        float vw0 = (4.0 - 3.0 * t);\n"
"        float vw1 = 7.0;\n"
"        float vw2 = (1.0 + 3.0 * t);\n"
"\n"
"        float v0 = (3.0 - 2.0 * t) / vw0 - 2.0;\n"
"        float v1 = (3.0 + t) / vw1;\n"
"        float v2 = t / vw2 + 2;\n"
"\n"
"        sum += uw0 * vw0 * SAMPLE_SHADOW_MAP(u0, v0);\n"
"        sum += uw1 * vw0 * SAMPLE_SHADOW_MAP(u1, v0);\n"
"        sum += uw2 * vw0 * SAMPLE_SHADOW_MAP(u2, v0);\n"
"\n"
"        sum += uw0 * vw1 * SAMPLE_SHADOW_MAP(u0, v1);\n"
"        sum += uw1 * vw1 * SAMPLE_SHADOW_MAP(u1, v1);\n"
"        sum += uw2 * vw1 * SAMPLE_SHADOW_MAP(u2, v1);\n"
"\n"
"        sum += uw0 * vw2 * SAMPLE_SHADOW_MAP(u0, v2);\n"
"        sum += uw1 * vw2 * SAMPLE_SHADOW_MAP(u1, v2);\n"
"        sum += uw2 * vw2 * SAMPLE_SHADOW_MAP(u2, v2);\n"
"\n"
"        return sum * 1.0 / 144.0;\n"
"\n"
"    #elif SHADOW_FILTER_SIZE == 7\n"
"\n"
"        float uw0 = (5.0 * s - 6.0);\n"
"        float uw1 = (11.0 * s - 28.0);\n"
"        float uw2 = -(11.0 * s + 17.0);\n"
"        float uw3 = -(5.0 * s + 1.0);\n"
"\n"
"        float u0 = (4.0 * s - 5.0) / uw0 - 3.0;\n"
"        float u1 = (4.0 * s - 16.0) / uw1 - 1.0;\n"
"        float u2 = -(7.0 * s + 5.0) / uw2 + 1.0;\n"
"        float u3 = -s / uw3 + 3.0;\n"
"\n"
"        float vw0 = (5.0 * t - 6.0);\n"
"        float vw1 = (11.0 * t - 28.0);\n"
"        float vw2 = -(11.0 * t + 17.0);\n"
"        float vw3 = -(5.0 * t + 1.0);\n"
"\n"
"        float v0 = (4.0 * t - 5.0) / vw0 - 3.0;\n"
"        float v1 = (4.0 * t - 16.0) / vw1 - 1.0;\n"
"        float v2 = -(7.0 * t + 5.0) / vw2 + 1.0;\n"
"        float v3 = -t / vw3 + 3.0;\n"
"\n"
"        sum += uw0 * vw0 * SAMPLE_SHADOW_MAP(u0, v0);\n"
"        sum += uw1 * vw0 * SAMPLE_SHADOW_MAP(u1, v0);\n"
"        sum += uw2 * vw0 * SAMPLE_SHADOW_MAP(u2, v0);\n"
"        sum += uw3 * vw0 * SAMPLE_SHADOW_MAP(u3, v0);\n"
"\n"
"        sum += uw0 * vw1 * SAMPLE_SHADOW_MAP(u0, v1);\n"
"        sum += uw1 * vw1 * SAMPLE_SHADOW_MAP(u1, v1);\n"
"        sum += uw2 * vw1 * SAMPLE_SHADOW_MAP(u2, v1);\n"
"        sum += uw3 * vw1 * SAMPLE_SHADOW_MAP(u3, v1);\n"
"\n"
"        sum += uw0 * vw2 * SAMPLE_SHADOW_MAP(u0, v2);\n"
"        sum += uw1 * vw2 * SAMPLE_SHADOW_MAP(u1, v2);\n"
"        sum += uw2 * vw2 * SAMPLE_SHADOW_MAP(u2, v2);\n"
"        sum += uw3 * vw2 * SAMPLE_SHADOW_MAP(u3, v2);\n"
"\n"
"        sum += uw0 * vw3 * SAMPLE_SHADOW_MAP(u0, v3);\n"
"        sum += uw1 * vw3 * SAMPLE_SHADOW_MAP(u1, v3);\n"
"        sum += uw2 * vw3 * SAMPLE_SHADOW_MAP(u2, v3);\n"
"        sum += uw3 * vw3 * SAMPLE_SHADOW_MAP(u3, v3);\n"
"\n"
"        return sum * 1.0 / 2704.0;\n"
"    #else\n"
"        return 0.0;\n"
"    #endif\n"
"#undef SAMPLE_SHADOW_MAP\n"
"}\n"
"\n"
"\n"
"\n"
"float ComputeShadowAmount(ShadowMapAttribs          ShadowAttribs,\n"
"                          in Texture2DArray<float>  tex2DShadowMap,\n"
"                          in SamplerComparisonState tex2DShadowMap_sampler,\n"
"                          in float3                 f3PosInLightViewSpace,\n"
"                          in float                  fCameraSpaceZ,\n"
"                          out int                   Cascade)\n"
"{\n"
"    float3 f3PosInCascadeProjSpace  = float3(0.0, 0.0, 0.0);\n"
"    float3 f3CascadeLightSpaceScale = float3(0.0, 0.0, 0.0);\n"
"    FindCascade(ShadowAttribs, f3PosInLightViewSpace.xyz, fCameraSpaceZ, f3PosInCascadeProjSpace, f3CascadeLightSpaceScale, Cascade);\n"
"    if( Cascade == ShadowAttribs.iNumCascades )\n"
"        return 1.0;\n"
"\n"
"    float3 f3ShadowMapUVDepth;\n"
"    f3ShadowMapUVDepth.xy = NormalizedDeviceXYToTexUV( f3PosInCascadeProjSpace.xy );\n"
"    f3ShadowMapUVDepth.z = NormalizedDeviceZToDepth( f3PosInCascadeProjSpace.z );\n"
"        \n"
"    float3 f3ddXShadowMapUVDepth = ddx(f3PosInLightViewSpace) * f3CascadeLightSpaceScale * F3NDC_XYZ_TO_UVD_SCALE;\n"
"    float3 f3ddYShadowMapUVDepth = ddy(f3PosInLightViewSpace) * f3CascadeLightSpaceScale * F3NDC_XYZ_TO_UVD_SCALE;\n"
"\n"
"    float2 f2DepthSlopeScaledBias = ComputeReceiverPlaneDepthBias(f3ddXShadowMapUVDepth, f3ddYShadowMapUVDepth);\n"
"    float2 SlopeScaledBiasClamp = float2(ShadowAttribs.ReceiverPlaneDepthBiasClamp, ShadowAttribs.ReceiverPlaneDepthBiasClamp);\n"
"    f2DepthSlopeScaledBias = clamp(f2DepthSlopeScaledBias, -SlopeScaledBiasClamp, SlopeScaledBiasClamp);\n"
"    uint SMWidth, SMHeight, Elems; \n"
"    tex2DShadowMap.GetDimensions(SMWidth, SMHeight, Elems);\n"
"    float2 ShadowMapDim = float2(SMWidth, SMHeight);\n"
"    f2DepthSlopeScaledBias /= ShadowMapDim.xy;\n"
"\n"
"    float fractionalSamplingError = dot( float2(1.0, 1.0), abs(f2DepthSlopeScaledBias.xy) );\n"
"    fractionalSamplingError = max(fractionalSamplingError, 1e-5);\n"
"    f3ShadowMapUVDepth.z -= fractionalSamplingError;\n"
"\n"
"    return FilterShadowMapOptimizedPCF(tex2DShadowMap, tex2DShadowMap_sampler, ShadowMapDim, f3ShadowMapUVDepth, Cascade, f2DepthSlopeScaledBias);\n"
"}\n"
"\n"
"float3 GetCascadeColor(int Cascade)\n"
"{\n"
"    float3 f3CascadeColors[MAX_CASCADES];\n"
"    f3CascadeColors[0] = float3(0,1,0);\n"
"    f3CascadeColors[1] = float3(0,0,1);\n"
"    f3CascadeColors[2] = float3(1,1,0);\n"
"    f3CascadeColors[3] = float3(0,1,1);\n"
"    f3CascadeColors[4] = float3(1,0,1);\n"
"    f3CascadeColors[5] = float3(0.3, 1, 0.7);\n"
"    f3CascadeColors[6] = float3(0.7, 0.3,1);\n"
"    f3CascadeColors[7] = float3(1, 0.7, 0.3);\n"
"    return f3CascadeColors[min(Cascade, MAX_CASCADES-1)];\n"
"}\n"
"\n"
"#endif //_SHADOWS_FXH_\n"