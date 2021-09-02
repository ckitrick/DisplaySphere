
/*
void ds_draw_geometry_new(DS_CTX *ctx);

//-----------------------------------------------------------------------------
void ds_draw_geometry(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	static int				i, j, k, reverseOrder;
	static MTX_MATRIX		*mp, mtx, rmtx, tmtx, nmtx;
	static MTX_VECTOR		*v;
	static MTX_VECTOR		*nml;
	static GUT_POINT		p[32];
	static GUT_POINT		q[32];
	static GUT_POINT		n[32];
	static GUT_POINT		cntr;
	double					maxZ;
	static GUT_VECTOR		vab, vbc, normal;
	static DS_POLYHEDRON	*poly;
	static DS_GEO_OBJECT	*gobj;
	static DS_FACE			*face;
	static DS_COLOR			*clr, color;
	static DS_EDGE			*edge;
	static GUT_POINT		out[24]; // edge triangle points 
	static GUT_POINT		origin[2] = { 0,0,0,1 };
	int						nSeg = 12;
	static GUT_VECTOR		edgeNormal[24];
	int						objectID = 0, matrixID = 0, transformID = 0;
	int						transparentFlag, skipFaceFlag = 0;
	float					alpha;
	double					scale;
	//	char					buf[12];

	return ds_draw_geometry_new(ctx);

	if (ctx->drawAdj.axiiFlag)
	{
		glPushMatrix();
		glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
		glMatrixMode(GL_MODELVIEW);
		glMultMatrixd((double*)&ctx->matrix);
		ds_gl_render_axii(ctx, p, out, &origin[0]);
		glPopMatrix();
	}

	mtx_create_translation_matrix(&rmtx, (double)ctx->trans[0], (double)ctx->trans[1], (double)ctx->trans[2]);
	mtx_vector_multiply(1, (MTX_VECTOR*)&origin[0], (MTX_VECTOR*)&origin[1], &rmtx); // transform the vertices once

	ds_blend_sort_init(ctx); // initialize transparency 

	LL_SetHead(ctx->gobjectq);
	while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
	{
		if (!gobj->active) continue;

		if (!gobj->v_out) gobj->v_out = malloc(sizeof(MTX_VECTOR) * gobj->nVtx); // alocate memory once 
		if (!gobj->n_out) gobj->n_out = malloc(sizeof(MTX_VECTOR) * gobj->nVtx); // alocate memory once 

		transformID = 0;
		ds_geometry_draw_init(ctx, gobj); // initialize transformations unque to each object

		while (ds_geometry_next_draw_transform(ctx, gobj, &mp, &reverseOrder, gobj->geo_orientation))
		{
			gobj->matrixID = matrixID = (objectID << 8) | (transformID++ & 0xff); // associate the current matrix with the object

			mtx_multiply_matrix(mp, &ctx->matrix, &mtx);
			mtx_multiply_matrix(&mtx, &rmtx, &tmtx);
			mtx_vector_multiply(gobj->nVtx, (MTX_VECTOR*)gobj->vtx, gobj->v_out, &tmtx); // transform the vertices once
			if (gobj->nmlFlag)
			{
				mtx_vector_multiply(gobj->nVtx, (MTX_VECTOR*)gobj->nml, gobj->n_out, &mtx); // transform the normals once
			}

			if (gobj->nTri && (gobj->drawWhat & GEOMETRY_DRAW_FACES)) // faces
			{

				for (i = 0, v = gobj->v_out, nml = gobj->n_out, face = gobj->tri; i < gobj->nTri; ++i, ++face)
				{
					switch (gobj->cAttr.face.state) {
					case DS_COLOR_STATE_EXPLICIT: clr = &face->color; break;
					case DS_COLOR_STATE_AUTOMATIC:ds_ctbl_get_color(gobj->ctT, face->id, &clr);  break;
					case DS_COLOR_STATE_OVERRIDE: clr = &gobj->cAttr.face.color; break;
					}
					if (transparentFlag = gobj->tAttr.onFlag)
					{
						switch (gobj->tAttr.state) {
						case DS_COLOR_STATE_EXPLICIT: alpha = clr->a; break; // face->color.a;		break;
						case DS_COLOR_STATE_OVERRIDE: alpha = gobj->tAttr.alpha;	break;
						}
						if (alpha == 0.0)
							skipFaceFlag = 1; // nothing to draw
						else if (alpha == 1.0)
							transparentFlag = 0; // fully opaque
					}
					switch (face->nVtx) {
					case 1: // degenerate vertices
						if (ctx->eAttr.maxLength > 0.0)		scale = gobj->vAttr.scale * ctx->eAttr.maxLength;
						else								scale = gobj->vAttr.scale;
						ds_gl_render_vertex(ctx, (GUT_POINT*)&v[face->vtx[0]], ctx->renderVertex.vtxObj->vtx, (GUT_POINT*)ctx->renderVertex.vtxObj->v_out, ctx->renderVertex.vtxObj->tri, ctx->renderVertex.vtxObj->nVtx, ctx->renderVertex.vtxObj->nTri, scale, clr, &origin[1]);
						if (gobj->lFlags.face)
						{
							cntr = *(GUT_POINT*)&v[face->vtx[0]];
							normal.i = cntr.x - origin[1].x;
							normal.j = cntr.y - origin[1].y;
							normal.k = cntr.z - origin[1].z;
							gut_normalize_vector(&normal);
							glNormal3f((float)normal.i, (float)normal.j, (float)normal.k);
							ds_label_draw_id(&ctx->label.face, (float)cntr.x, (float)cntr.y, (float)cntr.z + scale * 1.25, (float)normal.k, face->id);
						}
						break;
					case 2: // degenerate edges
						ds_gl_render_edge(ctx, gobj, (GUT_POINT*)&v[face->vtx[0]].data.xyzw[0], (GUT_POINT*)&v[face->vtx[1]].data.xyzw[0], p, &origin[1], out, edgeNormal, clr, ctx->drawAdj.quality->edgeNSeg);
						if (gobj->lFlags.face)
						{
							gut_mid_point((GUT_POINT*)&v[face->vtx[0]].data.xyzw[0], (GUT_POINT*)&v[face->vtx[1]].data.xyzw[0], &cntr);
							normal.i = cntr.x - origin[1].x;
							normal.j = cntr.y - origin[1].y;
							normal.k = cntr.z - origin[1].z;
							gut_normalize_vector(&normal);
							glNormal3f((float)normal.i, (float)normal.j, (float)normal.k);
							ds_label_draw_id(&ctx->label.face, (float)cntr.x, (float)cntr.y, (float)cntr.z + ctx->eAttr.maxLength * gobj->eAttr.width * 1.5, (float)normal.k, face->id);
						}
						break;
					default: // normal polygon faces
						if (transparentFlag) //gobj->cAttr.face.transparencyFlag)
						{
							ds_blend_sort_one_face(ctx, gobj, face, &tmtx, &mtx, reverseOrder, matrixID, alpha);  // add face to transparency context
						}
						else if (!skipFaceFlag) // draw
						{
							if (!reverseOrder) // copy vertex data to new variables
								for (j = 0, cntr.x = 0, cntr.y = 0, cntr.z = 0, maxZ = (-1000); j < face->nVtx; ++j)
								{
									p[j] = *(GUT_POINT*)&v[face->vtx[j]].data.xyzw[0];
									if (gobj->nmlFlag)
										n[j] = *(GUT_POINT*)&nml[face->nml[j]].data.xyzw[0];
									cntr.x += p[j].x; cntr.y += p[j].y; cntr.z += p[j].z;
									//									maxZ = p[j].z > maxZ ? p[j].z : maxZ;
								}
							else
								for (k = 0, j = face->nVtx - 1, cntr.x = 0, cntr.y = 0, cntr.z = 0, maxZ = (-1000); j >= 0; --j, ++k)
								{
									p[k] = *(GUT_POINT*)&v[face->vtx[j]].data.xyzw[0];
									if (gobj->nmlFlag)
										n[j] = *(GUT_POINT*)&nml[face->nml[j]].data.xyzw[0];
									cntr.x += p[k].x; cntr.y += p[k].y; cntr.z += p[k].z;
									//									maxZ = p[k].z > maxZ ? p[k].z : maxZ;
								}
							cntr.x /= face->nVtx;
							cntr.y /= face->nVtx;
							cntr.z /= face->nVtx;
							// check for special flag to re-normalize
							if (ctx->drawAdj.normalizeFlag)//if (ctx->global_normalize)
								for (j = 0; j < face->nVtx; ++j) ds_normalize_point(&origin[1], &p[j]);

							// determine face normal from cross product
							gut_vector(&p[0], &p[1], &vab);
							gut_vector(&p[1], &p[2], &vbc);
							gut_cross_product(&vab, &vbc, &normal);
							gut_normalize_point((GUT_POINT*)&normal);

							// check face options
							gobj->fAttr.extrusion.enable = 0;
							gobj->fAttr.offset.enable = 0;
							gobj->fAttr.size.enable = 0;
							gobj->fAttr.hole.enable = 0;

							if (face->nVtx == 3 && ctx->drawAdj.circleFlag)
							{
								//							glBegin(GL_TRIANGLES);
								color = *clr;
								color.a = 1.0;
								ds_draw_circle_segment(&p[0], &p[1], &p[2], &normal, clr, &origin[1]);
							}
							else
							{
								if (!gobj->nmlFlag)
								{
									glBegin(GL_TRIANGLES);
									for (j = 2; j < face->nVtx; ++j)
									{
										// determine face normal from cross product
										gut_vector(&p[0], &p[j - 1], &vab);
										gut_vector(&p[j - 1], &p[j], &vbc);
										gut_cross_product(&vab, &vbc, &normal);
										gut_normalize_point((GUT_POINT*)&normal);
										glNormal3f((float)normal.i, (float)normal.j, (float)normal.k);
										glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
										glVertex3f((float)p[0].x, (float)p[0].y, (float)p[0].z);
										glVertex3f((float)p[j - 1].x, (float)p[j - 1].y, (float)p[j - 1].z);
										glVertex3f((float)p[j].x, (float)p[j].y, (float)p[j].z);
									}
									glEnd();
								}
								else
								{	// use owner supplied normals
									glBegin(GL_TRIANGLES);
									for (j = 2; j < face->nVtx; ++j)
									{
										glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
										glNormal3f((float)n[0].x, (float)n[0].y, (float)n[0].z);
										glVertex3f((float)p[0].x, (float)p[0].y, (float)p[0].z);
										glNormal3f((float)n[j - 1].x, (float)n[j - 1].y, (float)n[j - 1].z);
										glVertex3f((float)p[j - 1].x, (float)p[j - 1].y, (float)p[j - 1].z);
										glNormal3f((float)n[j].x, (float)n[j].y, (float)n[j].z);
										glVertex3f((float)p[j].x, (float)p[j].y, (float)p[j].z);
									}
									glEnd();
								}
							}
							if (gobj->lFlags.face)
								ds_label_draw_id(&ctx->label.face, (float)cntr.x, (float)cntr.y, (float)cntr.z + 0.03, (float)normal.k, face->id);
						}
					}
				}
			}

			if (gobj->nEdge && (gobj->drawWhat & GEOMETRY_DRAW_EDGES))
			{
				for (i = 0, v = gobj->v_out, edge = gobj->edge; i < gobj->nEdge; ++i, ++edge)
				{
					switch (gobj->cAttr.edge.state) {
					case DS_COLOR_STATE_EXPLICIT: clr = &face->color; break;
					case DS_COLOR_STATE_AUTOMATIC:ds_ctbl_get_color(gobj->ctE, edge->id, &clr);  break;
					case DS_COLOR_STATE_OVERRIDE: clr = &gobj->cAttr.edge.color; break;
					}

					ds_gl_render_edge(ctx, gobj, (GUT_POINT*)&v[edge->vtx[0]].data.xyzw, (GUT_POINT*)&v[edge->vtx[1]].data.xyzw, p, &origin[1], out, edgeNormal, clr,
						ctx->drawAdj.quality->edgeNSeg);

					if (gobj->lFlags.edge)
					{
						gut_mid_point((GUT_POINT*)&v[edge->vtx[0]].data.xyzw, (GUT_POINT*)&v[edge->vtx[1]].data.xyzw, &cntr);
						normal.i = cntr.x - origin[1].x;
						normal.j = cntr.y - origin[1].y;
						normal.k = cntr.z - origin[1].z;
						gut_normalize_vector(&normal);
						glNormal3f((float)normal.i, (float)normal.j, (float)normal.k);
						ds_label_draw_id(&ctx->label.edge, (float)cntr.x, (float)cntr.y, (float)cntr.z + ctx->eAttr.maxLength * gobj->eAttr.width * 1.5, (float)normal.k, edge->id);
					}
				}
			}

			if (gobj->drawWhat & GEOMETRY_DRAW_VERTICES)//0x4) // render vertices if required
			{
				double		scale;

				clr = &gobj->cAttr.vertex.color;

				if (ctx->eAttr.maxLength > 0.0)	scale = gobj->vAttr.scale * ctx->eAttr.maxLength;
				else								scale = gobj->vAttr.scale;

				for (i = 0, v = gobj->v_out; i < gobj->nVtx; ++i, ++face, ++v)
				{
					ds_gl_render_vertex(ctx, (GUT_POINT*)v, (GUT_POINT*)ctx->renderVertex.vtxObj->vtx, (GUT_POINT*)ctx->renderVertex.vtxObj->v_out, ctx->renderVertex.vtxObj->tri, ctx->renderVertex.vtxObj->nVtx, ctx->renderVertex.vtxObj->nTri, scale, clr, &origin[1]);

					if (gobj->lFlags.vertex)
					{
						cntr = *(GUT_POINT*)v;
						normal.i = cntr.x - origin[1].x;
						normal.j = cntr.y - origin[1].y;
						normal.k = cntr.z - origin[1].z;
						gut_normalize_vector(&normal);
						glNormal3f((float)normal.i, (float)normal.j, (float)normal.k);
						ds_label_draw_id(&ctx->label.vertex, (float)cntr.x, (float)cntr.y, (float)cntr.z + scale * 1.25, (float)normal.k, i);
					}
				}
			}
		}

		++objectID;
	}

	{ // processing of transparent faces
		int		nNodes, nLevels;
		avl_info(ctx->transparency.avlZSort, &nNodes, &nLevels);
		if (nNodes)
		{
			ctx->origin = origin[1];
			// set up transparent blending
			glEnable(GL_ALPHA_TEST);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// traverse the avl tree of faces - back to front z ordered 
			avl_traverse_rtl(ctx->transparency.avlZSort, (void*)ctx, ds_blend_draw_face);

			// restore regular rendering without blending
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}
	}

	glBegin(GL_TRIANGLES);

	if (ctx->drawAdj.clipFlag && ctx->drawAdj.clipVisibleFlag)
	{
		glNormal3f((float)0, (float)0, (float)1.0);

		glColor3f((float)1, (float)1, (float)0);
		glVertex3f((float) 1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float) 1.05, (float) 1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)-1.05, (float) 1.05, (float)ctx->drawAdj.clipZValue);

		glVertex3f((float)-1.05, (float) 1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)-1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float) 1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
	}

	glEnd();
}
*/

enum {
	DS_STATIC = 10,
	DS_STATIC_GROUP_OBJECT = 50,
	DS_STATIC_GROUP_DRAW,
	DS_STATIC_GROUP_COLOR_OVERRIDE,
	DS_STATIC_GROUP_EDGE,
	DS_STATIC_GROUP_VERTEX,

	DS_STATIC_TEXT_VIS,
	DS_STATIC_TEXT_DRAW_FACE,
	DS_STATIC_TEXT_DRAW_EDGE,
	DS_STATIC_TEXT_DRAW_VERTEX,

	DS_STATIC_TEXT_COLOR_OVERRIDE_FACE,
	DS_STATIC_TEXT_COLOR_OVERRIDE_EDGE,
	DS_STATIC_TEXT_COLOR_OVERRIDE_VERTEX,
	DS_STATIC_TEXT_COLOR_SELECT_FACE,
	DS_STATIC_TEXT_COLOR_SELECT_EDGE,
	DS_STATIC_TEXT_COLOR_SELECT_VERTEX,

	DS_STATIC_TEXT_EDGE_ROUND,
	DS_STATIC_TEXT_EDGE_BOX,
	DS_STATIC_TEXT_EDGE_WIDTH,
	DS_STATIC_TEXT_EDGE_HEIGHT,
	DS_STATIC_TEXT_EDGE_OFFSET,

	DS_STATIC_TEXT_VERTEX_SCALE,

	DS_BASE_VISIBLE = 100,
	DS_BASE_ATTRIBUTES = 200,
	DS_BASE_OBJECT_NAME = 300,

	DS_BASE_NAME = 100,
	DS_BASE_ACTIVE = 200,
	DS_BASE_DRAW_FACE = 300,
	DS_BASE_DRAW_EDGE = 400,
	DS_BASE_DRAW_VERTEX = 500,
	DS_BASE_COLOR_FACE_USE_E = 600,
	DS_BASE_COLOR_FACE_USE_A = 700,
	DS_BASE_COLOR_FACE_USE_O = 800,
	DS_BASE_COLOR_FACE_SET = 900,
	DS_BASE_COLOR_TRANS_ON = 1000,
	DS_BASE_COLOR_TRANS_E = 1100,
	DS_BASE_COLOR_TRANS_O = 1200,
	DS_BASE_COLOR_TRANS_ALPHA = 1300,
	DS_BASE_COLOR_EDGE_USE_A = 1400,
	DS_BASE_COLOR_EDGE_USE_O = 1500,
	DS_BASE_COLOR_EDGE_SET = 1600,
	DS_BASE_COLOR_VERTEX_SET = 1700,
	DS_BASE_EDGE_ROUND = 1800,
	DS_BASE_EDGE_BOX = 1900,
	DS_BASE_EDGE_WIDTH = 2000,
	DS_BASE_EDGE_HEIGHT = 2100,
	DS_BASE_EDGE_OFFSET = 2200,
	DS_BASE_VERTEX_SCALE = 2300,
	DS_BASE_REPLICATE_FACE = 2400,
	DS_BASE_REPLICATE_Z = 2500,
	DS_BASE_REPLICATE_X = 2600,
	DS_BASE_LABEL_FACE = 2700,
	DS_BASE_LABEL_EDGE = 2800,
	DS_BASE_LABEL_VERTEX = 2900,
	DS_BASE_GEO_ORIENTATION = 3000,
};
/*
DS_OBJECT_CONTROL ds_obj_fixed[] = {
	L"Button",	L"Object",			DS_STATIC,		  3, 2, 91,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  //   3, 2, 75,38
	L"Button",	L"Draw",			DS_STATIC,		 96, 2, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  //  82, 2, 40,38
	L"Button",	L"Color",			DS_STATIC,		139, 2,195,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 126, 2,199,38
	L"Button",	L"Face",			DS_STATIC,		139,13, 54,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 126,13, 55,38
	L"Button",	L"Transparency",	DS_STATIC,		195,13, 62,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 184,13, 62,38
	L"Button",	L"Edge",			DS_STATIC,		259,13, 44,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 249,13, 44,38
	L"Button",	L"Vertex",			DS_STATIC,		305,13, 29,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 296,13, 29,38
	L"Button",	L"Edge",			DS_STATIC,		336, 2,103,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 329, 2,106,38
	L"Button",	L"Vertex",			DS_STATIC,		441, 2, 31,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 438, 2, 31,38
	L"Button",	L"Replicate",		DS_STATIC,		474, 2, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 472, 2, 41,38
	L"Button",	L"Labels",			DS_STATIC,		517, 2, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 517, 2, 41,38

	L"Static",	L"NAME",			DS_STATIC,		  8,22,65,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Vis",				DS_STATIC,		 55,22,10,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Geo",				DS_STATIC,		 70,22,20,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"F",				DS_STATIC,		102,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		114,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"V",				DS_STATIC,		126,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"E",				DS_STATIC,		146,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"A",				DS_STATIC,		157,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		168,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"On",				DS_STATIC,		199,22, 9,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		213,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		224,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"alpha",			DS_STATIC,		236,22,18,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"A",				DS_STATIC,		265,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		276,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"R",				DS_STATIC,		342,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"B",				DS_STATIC,		352,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Width",			DS_STATIC,		363,20,20,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Height",			DS_STATIC,		389,20,22,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Offset",			DS_STATIC,		414,20,22,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"Scale",			DS_STATIC,		447,20,18,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"1F",				DS_STATIC,		479,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Z",				DS_STATIC,		493,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"X",				DS_STATIC,		503,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"F",				DS_STATIC,		524,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		535,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"V",				DS_STATIC,		547,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
};

int nDS_Fixed_Controls = sizeof(ds_obj_fixed) / sizeof(DS_OBJECT_CONTROL);
*/
/*
	DS_OBJECT_CONTROL_EX ds_obj_variable[] = {
	(LPCWSTR)WC_STATIC,  L"default",DS_BASE_NAME				,	  8,32,40, 8,	WS_VISIBLE | WS_CHILD | SS_LEFT,								0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_ACTIVE					,	 55,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,

	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_FACE				,	100,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_EDGE				,	112,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_VERTEX				,	124,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	// face color
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_E		,	144,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,	0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_A		,	155,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_O		,	166,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_SET			,	177,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// transparency
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_TRANS_ON			,	199,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_TRANS_E			,	211,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,	0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_TRANS_O			,	222,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_COLOR_TRANS_ALPHA		,	233,31,21,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// edge color
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_USE_A		,	263,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_USE_O		,	274,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_SET			,	286,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// vertex color 
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_VERTEX_SET		,	313,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Edge round/box size
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_EDGE_ROUND				,	340,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_EDGE_BOX				,	350,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_WIDTH				,	361,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_HEIGHT				,	387,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_OFFSET				,	412,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Vertex Scale
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_VERTEX_SCALE			,	445,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Replication 
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_FACE			,	479,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_Z				,	492,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_X				,	502,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	// Replication 
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_LABEL_FACE				,	521,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_LABEL_EDGE				,	533,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_LABEL_VERTEX			,	545,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,

	(LPCWSTR)WC_COMBOBOX,L"",	DS_BASE_GEO_ORIENTATION 		,	 65,31,25,200,	SS_SIMPLE | CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,			0,
};
//CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,

int nDS_Variable_Controls = sizeof(ds_obj_variable) / sizeof(DS_OBJECT_CONTROL_EX);
*/
/*
//-----------------------------------------------------------------------------
static void ds_fill_object_controls(HWND hWndDlg, int objID, DS_GEO_OBJECT *gobj, char *buffer)
//-----------------------------------------------------------------------------
{
	if (gobj->name[0])
	{
		SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, gobj->name);
	}
	else if (gobj->filename)
	{
		SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, ds_name_start(gobj->filename));
	}
	else
	{
		SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, "default");
	}
	SendDlgItemMessage(hWndDlg, DS_BASE_ACTIVE + objID, BM_SETCHECK, (gobj->active ? BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_DRAW_FACE + objID, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_TRIANGLES ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_DRAW_EDGE + objID, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_EDGES ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_DRAW_VERTEX + objID, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_VERTICES ? BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_FACE_USE_E + objID, BM_SETCHECK, (gobj->cAttr.face.state & DS_COLOR_STATE_EXPLICIT ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_FACE_USE_A + objID, BM_SETCHECK, (gobj->cAttr.face.state & DS_COLOR_STATE_AUTOMATIC ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_FACE_USE_O + objID, BM_SETCHECK, (gobj->cAttr.face.state & DS_COLOR_STATE_OVERRIDE ? BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_TRANS_ON + objID, BM_SETCHECK, (gobj->tAttr.onFlag ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_TRANS_E + objID, BM_SETCHECK, (gobj->tAttr.state & DS_COLOR_STATE_EXPLICIT ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_TRANS_O + objID, BM_SETCHECK, (gobj->tAttr.state & DS_COLOR_STATE_OVERRIDE ? BST_CHECKED : BST_UNCHECKED), 0);
	sprintf(buffer, "%.2f", gobj->tAttr.alpha);  SetDlgItemText(hWndDlg, DS_BASE_COLOR_TRANS_ALPHA + objID, buffer);

	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_EDGE_USE_A + objID, BM_SETCHECK, (gobj->cAttr.edge.state & DS_COLOR_STATE_AUTOMATIC ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_EDGE_USE_O + objID, BM_SETCHECK, (gobj->cAttr.edge.state & DS_COLOR_STATE_OVERRIDE ? BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_EDGE_ROUND + objID, BM_SETCHECK, (gobj->eAttr.type & GEOMETRY_EDGE_ROUND ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_EDGE_BOX + objID, BM_SETCHECK, (gobj->eAttr.type & GEOMETRY_EDGE_SQUARE ? BST_CHECKED : BST_UNCHECKED), 0);

	sprintf(buffer, "%.3f", gobj->eAttr.width);  SetDlgItemText(hWndDlg, DS_BASE_EDGE_WIDTH + objID, buffer);
	sprintf(buffer, "%.3f", gobj->eAttr.height); SetDlgItemText(hWndDlg, DS_BASE_EDGE_HEIGHT + objID, buffer);
	sprintf(buffer, "%.3f", gobj->eAttr.offset); SetDlgItemText(hWndDlg, DS_BASE_EDGE_OFFSET + objID, buffer);
	sprintf(buffer, "%.3f", gobj->vAttr.scale);  SetDlgItemText(hWndDlg, DS_BASE_VERTEX_SCALE + objID, buffer);

	SendDlgItemMessage(hWndDlg, DS_BASE_REPLICATE_FACE + objID, BM_SETCHECK, (gobj->rAttr.oneFaceFlag ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_REPLICATE_Z + objID, BM_SETCHECK, (gobj->rAttr.zRotationFlag ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_REPLICATE_X + objID, BM_SETCHECK, (gobj->rAttr.xMirrorFlag ? BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_LABEL_FACE + objID, BM_SETCHECK, (gobj->lFlags.face ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_LABEL_EDGE + objID, BM_SETCHECK, (gobj->lFlags.edge ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_LABEL_VERTEX + objID, BM_SETCHECK, (gobj->lFlags.vertex ? BST_CHECKED : BST_UNCHECKED), 0);

	{
		char *geo_orientation[12] =
		{
			"IF - Icosahedron Face",
			"IE - Icosahedron Edge",
			"IV - Icosahedron Vertex",
			"OF - Octahedron Face",
			"OE - Octahedron Edge",
			"OV - Octahedron Vertex",
			"TF - Tetrahedron Face",
			"TE - Tetrahedron Edge",
			"TV - Tetrahedron Vertex",
			"CF - Cube Face",
			"CE - Cube Edge",
			"CV - Cube Vertex",
		};

		int i;
		for (i = 0; i < 12; ++i)
		{
			SendDlgItemMessage(hWndDlg, DS_BASE_GEO_ORIENTATION + objID, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)geo_orientation[i]);
		}

		// Send the CB_SETCURSEL message to display an initial item 
		//  in the selection field  
		int		index;
		index = (gobj->geo_type - 1) * 3 + (2 - gobj->geo_orientation);
		SendDlgItemMessage(hWndDlg, DS_BASE_GEO_ORIENTATION + objID, CB_SETCURSEL, (WPARAM)index, (LPARAM)0);
		SendDlgItemMessage(hWndDlg, DS_BASE_GEO_ORIENTATION + objID, CB_SETDROPPEDWIDTH, (WPARAM)132, (LPARAM)0);
	}
}


//-----------------------------------------------------------------------------
void static ds_draw_variable_controls(HWND hWndDlg, HFONT s_hFont, int yOffset, int *objID, int *bottom, int defaultObject)
//-----------------------------------------------------------------------------
{
	int						i;
	DS_OBJECT_CONTROL_EX	*dsox;
	HWND					hEdit;
	RECT					rect;

	// do this for each geometry object  
	*bottom = 0;

	for (i = 0, dsox = ds_obj_variable; i < nDS_Variable_Controls; ++i, ++dsox)
	{
		rect.top = dsox->y + yOffset * *objID;
		rect.left = dsox->x;
		rect.right = dsox->x + dsox->w;
		rect.bottom = dsox->y + dsox->h + yOffset * *objID;
		if (dsox->h < 40 && rect.bottom > *bottom)
			*bottom = rect.bottom;
		MapDialogRect(hWndDlg, &rect);

		if (!defaultObject || (defaultObject && i != 1)) // don't create window for default visible checkbox
		{
			hEdit = CreateWindowExA(dsox->exStyle | WS_EX_TOOLWINDOW, (LPCSTR)dsox->className, (LPCSTR)dsox->text, dsox->style,
				//			hEdit = CreateWindowExA(dsox->exStyle, (LPCSTR)dsox->className, (LPCSTR)dsox->text, dsox->style,
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hWndDlg,
				(HMENU)(dsox->id + *objID), GetModuleHandle(NULL), NULL);
			if (hEdit)
				SendMessage(hEdit, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
		}
	}
	++*objID;
}
*/

/*
//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_object_control(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	HWND					pWnd;
	DS_CTX					*ctx;
	static HFONT			s_hFont = NULL;
	HWND					hEdit;
	const TCHAR*			fontName = _T("Microsoft Sans Serif");
	const long				nFontSize = 8;
	LOGFONT					logFont = { 0 };
	HDC						hdc = GetDC(hWndDlg);
	int						yOffset = 15;
	RECT					rect; 
	DS_GEO_OBJECT			*gobj;
	int						temp, clrUpdate;
	char					buffer[128]; 
	int						control, category, objID;
	static DS_GEO_OBJECT	gobj_def;
	static PAINTSTRUCT		ps;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);

	switch (Msg) {
	case WM_INITDIALOG:
		logFont.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		logFont.lfWeight = FW_REGULAR;
		strcpy_s(logFont.lfFaceName, 32, fontName);
		s_hFont = CreateFontIndirect(&logFont);
		ReleaseDC(hWndDlg, hdc);

		int						i, nObj, bottom, maxBottom;
		DS_OBJECT_CONTROL		*dso;
		// do this for each geometry object  
		objID = 0;
		bottom = 0;

		// default object settings
		memcpy(&gobj_def, &ctx->defInputObj, sizeof(DS_GEO_INPUT_OBJECT));
		ds_draw_variable_controls(hWndDlg, s_hFont, yOffset, &objID, &bottom, 1);

		// loop thru real objects
		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
			ds_draw_variable_controls( hWndDlg, s_hFont, yOffset, &objID, &bottom, 0);
		}
		nObj = LL_GetLength(ctx->gobjectq) + 1; // add default
		maxBottom = 0;
		for (i = 0, dso = ds_obj_fixed; i < nDS_Fixed_Controls; ++i, ++dso)
		{
			rect.top = dso->y;
			rect.left = dso->x;
			rect.right = dso->x + dso->w;
			rect.bottom = dso->y + dso->h;
			if (dso->style & BS_GROUPBOX)
			{
				rect.bottom = bottom + 5;
			}
			MapDialogRect(hWndDlg, &rect);
			if (rect.bottom > maxBottom)
				maxBottom = rect.bottom;
			hEdit = CreateWindowW((LPCWSTR)dso->className, dso->text, dso->style,
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
				hWndDlg, (HMENU)dso->id, GetModuleHandle(NULL), NULL);
			SendMessage(hEdit, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
		}
		{ // resize dialog to match the size based on the number of objects
			RECT			window;
			GetWindowRect(hWndDlg, &window);
			int  w, h;
			w = window.right - window.left;
			h = window.bottom - window.top;
			h = maxBottom - 10;
			h += WINDOW_SIZE_OFFSET_HEIGHT;
			MoveWindow(hWndDlg, window.left, window.top, w, h, 1);// need to add extra
		}
		objID = 0;

		// fill in the default
		ds_fill_object_controls(hWndDlg, objID, &gobj_def, buffer);

		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{// send messages to all controls to update content
			ds_fill_object_controls(hWndDlg, ++objID, gobj, buffer);
		}
		break;

	case WM_PAINT:
		BeginPaint(hWndDlg, &ps);
//
//		objID = 0;
//
//		// fill in the default
//		ds_fill_object_controls(hWndDlg, objID, &gobj_def, buffer);
//
//		LL_SetHead(ctx->gobjectq);
//		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
//		{// send messages to all controls to update content
//			ds_fill_object_controls( hWndDlg, ++objID,  gobj, buffer);
//		}
//
		EndPaint(hWndDlg, &ps);
		break;

	case WM_COMMAND:
//		switch (wParam)
//		{
//		case IDOK:
//			ctx->objControl = 0;
//			DestroyWindow(hWndDlg);
//
//		case IDCANCEL:
//			ctx->objControl = 0;
//			DestroyWindow(hWndDlg);
//		}

		control   = LOWORD(wParam); // the specific control that triggered the event
		category  = control / 100;	// control category
		category *= 100;
		objID     = control % 100;	// object ID 
		temp      = 0;

		if (control < 100 || control > 3100)
			return FALSE;

		if (objID < 0 || objID > LL_GetLength(ctx->gobjectq))
			return FALSE;
		else if (!objID)
			gobj = (DS_GEO_OBJECT*)&ctx->defInputObj;
		else
		{
			temp = 1;
			LL_SetHead(ctx->gobjectq);
			while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
			{
				if (temp == objID) break;
				++temp;
			}
		}
		if (!gobj) 
			break; // done since we didn't find an object

		{
			UINT uiID = LOWORD(wParam);
			UINT uiCode = HIWORD(wParam);   // spin_update ( ctx, what, buffer )  what = 0,1,2
			switch (uiCode) {
			case EN_KILLFOCUS:
				switch (category) {
				case DS_BASE_EDGE_WIDTH:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->eAttr.width,  1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_EDGE_HEIGHT:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->eAttr.height, 1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_EDGE_OFFSET:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->eAttr.offset, 1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_VERTEX_SCALE:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->vAttr.scale,  1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_COLOR_TRANS_ALPHA: ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->tAttr.alpha,  0, 2, 1, 0.0, 1.0); break;
				}
				break;
			}
		}
		clrUpdate = 0;
		switch ( category ) {
		case DS_BASE_NAME: break;

		case DS_BASE_ACTIVE: gobj->active = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_DRAW_FACE:   gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_EDGES | GEOMETRY_DRAW_VERTICES)) | ( SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_FACES    : 0);  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_DRAW_EDGE:   gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_FACES | GEOMETRY_DRAW_VERTICES)) | ( SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_EDGES    : 0);  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_DRAW_VERTEX: gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_FACES | GEOMETRY_DRAW_EDGES))    | ( SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_VERTICES : 0);  InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_FACE_USE_E: gobj->cAttr.face.state = DS_COLOR_STATE_EXPLICIT;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_FACE_USE_A: gobj->cAttr.face.state = DS_COLOR_STATE_AUTOMATIC; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_FACE_USE_O: gobj->cAttr.face.state = DS_COLOR_STATE_OVERRIDE;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_FACE_SET:   clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.face.color);   InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_TRANS_ON: gobj->tAttr.onFlag = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_TRANS_E:  gobj->tAttr.state = DS_COLOR_STATE_EXPLICIT;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_TRANS_O:  gobj->tAttr.state = DS_COLOR_STATE_OVERRIDE;  InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_EDGE_USE_A: gobj->cAttr.edge.state = DS_COLOR_STATE_AUTOMATIC;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_EDGE_USE_O: gobj->cAttr.edge.state = DS_COLOR_STATE_OVERRIDE;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_EDGE_SET:   clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.edge.color);   InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_VERTEX_SET: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.vertex.color); InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_EDGE_ROUND: gobj->eAttr.type = GEOMETRY_EDGE_ROUND;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_EDGE_BOX:   gobj->eAttr.type = GEOMETRY_EDGE_SQUARE; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_REPLICATE_FACE:	gobj->rAttr.oneFaceFlag   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_REPLICATE_Z:		gobj->rAttr.zRotationFlag = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_REPLICATE_X:		gobj->rAttr.xMirrorFlag   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_LABEL_FACE:		gobj->lFlags.face   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_LABEL_EDGE:		gobj->lFlags.edge   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_LABEL_VERTEX:		gobj->lFlags.vertex = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_GEO_ORIENTATION:	//gobj->lFlags.vertex = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int ItemIndex = SendDlgItemMessage(hWndDlg, control, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				// decode index in geometry and orientation
				gobj->geo_type = ItemIndex / 3 + 1;
				gobj->geo_orientation = 2 - ItemIndex % 3;
				InvalidateRect(pWnd, 0, 0);
			}
			break;
		}
		if (clrUpdate)
		{
			clrUpdate = 0;
			LPDRAWITEMSTRUCT lpdis = (DRAWITEMSTRUCT*)lParam;
			InvalidateRect((HWND)lParam, 0, 0);
		}
		break;

	case WM_DESTROY:
	case WM_CLOSE:
		if (ctx->objDashboard)
		{
			DestroyWindow(hWndDlg);
			ctx->objDashboard = 0;
		}
		break;

	case WM_DRAWITEM: // owner drawn 
		{
			LPDRAWITEMSTRUCT	lpdis = (DRAWITEMSTRUCT*)lParam;
			RECT				lpr;
			HBRUSH				hColorBrush;
			DS_COLOR			*clr = 0;

			control = LOWORD(wParam);   // the specific control that triggered the event
			category = control / 100;	// control category
			category *= 100;
			objID = control % 100;	    // object ID 
			temp = 0;
			if (control < 100 || control > 2300)
				return FALSE;
			if (objID < 0 || objID > LL_GetLength(ctx->gobjectq))
				return FALSE;
			else if (!objID)
				gobj = (DS_GEO_OBJECT*)&ctx->defInputObj;
			else
			{
				temp = 1;
				LL_SetHead(ctx->gobjectq);
				while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
				{
					if (temp == objID) break;
					++temp;
				}
			}
			if (!gobj) break;;

			switch (category) {
			case DS_BASE_COLOR_FACE_SET:   clr = &gobj->cAttr.face.color;		break;
			case DS_BASE_COLOR_EDGE_SET:   clr = &gobj->cAttr.edge.color;		break;
			case DS_BASE_COLOR_VERTEX_SET: clr = &gobj->cAttr.vertex.color;		break;
			}
			if (clr)
			{
				hColorBrush = CreateSolidBrush(RGB((unsigned int)(clr->r * 255), (unsigned int)(clr->g * 255), (unsigned int)(clr->b * 255)));
				GetWindowRect(lpdis->hwndItem, &lpr);
				// convert to local coordinates
				lpr.right  = lpr.right - lpr.left;
				lpr.left   = 0;
				lpr.bottom = lpr.bottom - lpr.top;
				lpr.top    = 0;
				FillRect(lpdis->hDC, &lpr, hColorBrush);
			}
		}
		break;
	}

	return FALSE;
}
*/