/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeFont helper class implementation
*/


#include "..\..\include\hgefont.h"
#include <stdlib.h>
#include <stdio.h>

const char FNTHEADERTAG[] = "[HGEFONT]";
const char FNTBITMAPTAG[] = "Bitmap";
const char FNTCHARTAG[]   = "Char";


HGE *hgeFont::hge=0;
char hgeFont::buffer[1024];


hgeFont::hgeFont(const char *szFont, bool bMipmap)
{
	void	*data;
	DWORD	size;
	char	*desc, *pdesc;
	char	linebuf[256];
	char	buf[MAX_PATH], *pbuf;
	char	chr;
	int		i, x, y, w, h, a, c;

	// Setup variables
	
	hge=hgeCreate(HGE_VERSION);

	fHeight=0.0f;
	fScale=1.0f;
	fProportion=1.0f;
	fRot=0.0f;
	fTracking=0.0f;
	fSpacing=1.0f;
	hTexture=0;

	fZ=0.5f;
	nBlend=BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE;
	dwCol=0xFFFFFFFF;

	ZeroMemory( &letters, sizeof(letters) );
	ZeroMemory( &pre, sizeof(letters) );
	ZeroMemory( &post, sizeof(letters) );
	
	// Load font description

	data=hge->Resource_Load(szFont, &size);
	if(!data) return;

	desc = new char[size+1];
	memcpy(desc,data,size);
	desc[size]=0;
	hge->Resource_Free(data);

	pdesc=_get_line(desc,linebuf);
	if(strcmp(linebuf, FNTHEADERTAG))
	{
		hge->System_Log("Font %s has incorrect format.", szFont);
		delete[] desc;	
		return;
	}

	// Parse font description

	while(pdesc = _get_line(pdesc,linebuf))
	{
		if(!strncmp(linebuf, FNTBITMAPTAG, sizeof(FNTBITMAPTAG)-1 ))
		{
			strcpy(buf,szFont);
			pbuf=strrchr(buf,'\\');
			if(!pbuf) pbuf=strrchr(buf,'/');
			if(!pbuf) pbuf=buf;
			else pbuf++;
			if(!sscanf(linebuf, "Bitmap = %s", pbuf)) continue;

			hTexture=hge->Texture_Load(buf, 0, bMipmap);
			if(!hTexture)
			{
				delete[] desc;	
				return;
			}
		}

		else if(!strncmp(linebuf, FNTCHARTAG, sizeof(FNTCHARTAG)-1 ))
		{
			pbuf=strchr(linebuf,'=');
			if(!pbuf) continue;
			pbuf++;
			while(*pbuf==' ') pbuf++;
			if(*pbuf=='\"')
			{
				pbuf++;
				i=(unsigned char)*pbuf++;
				pbuf++; // skip "
			}
			else
			{
				i=0;
				while((*pbuf>='0' && *pbuf<='9') || (*pbuf>='A' && *pbuf<='F') || (*pbuf>='a' && *pbuf<='f'))
				{
					chr=*pbuf;
					if(chr >= 'a') chr-='a'-':';
					if(chr >= 'A') chr-='A'-':';
					chr-='0';
					if(chr>0xF) chr=0xF;
					i=(i << 4) | chr;
					pbuf++;
				}
				if(i<0 || i>255) continue;
			}
			sscanf(pbuf, " , %d , %d , %d , %d , %d , %d", &x, &y, &w, &h, &a, &c);

			letters[i] = new hgeSprite(hTexture, (float)x, (float)y, (float)w, (float)h);
			pre[i]=(float)a;
			post[i]=(float)c;
			if(h>fHeight) fHeight=(float)h;
		}
	}

	delete[] desc;	
}


hgeFont::~hgeFont()
{
	// 依次释放letters精灵数组（最大256个）
	// 每一个精灵即代表一个字符

	for(int i=0; i<256; i++)
		if(letters[i]) delete letters[i];
	// 释放字体贴图
	if(hTexture) hge->Texture_Free(hTexture);

	// 释放hge引用
	hge->Release();
}

void hgeFont::Render(float x, float y, int align, const char *string)
{
	int i;
	float	fx=x;

	// 首先将align位与HGETEXT_HORZMASK（水平掩码）
	align &= HGETEXT_HORZMASK;

	// 如果对齐方式为靠右，修正fx的坐标值
	if(align==HGETEXT_RIGHT) fx-=GetStringWidth(string, false);

	// 如果对齐方式为居中，修正fx的坐标值
	if(align==HGETEXT_CENTER) fx-=int(GetStringWidth(string, false)/2.0f);

	// 当前字符不为空 (/0)
	while(*string)
	{
		// 如果当前为换行符
		if(*string=='\n')
		{
			// 更行y坐标，注意计算公式，为 高度*缩放比例*行距比例
			y += int(fHeight*fScale*fSpacing);
			fx = x;
			// 根据对齐方式继续修正fx坐标
			if(align == HGETEXT_RIGHT)  fx -= GetStringWidth(string+1, false);
			if(align == HGETEXT_CENTER) fx -= int(GetStringWidth(string+1, false)/2.0f);
		}
		else
		{
			// 获取当前字符值
			i=(unsigned char)*string;
			// 如果当前精灵字符数组中找不到，便以 '?' 代替
			if(!letters[i]) i='?';
			if(letters[i])
			{
				// 更新fx坐标，注意计算公式，为 前坐标*缩放比例*字宽比例
				fx += pre[i]*fScale*fProportion;
				// 调用精灵（hgeSprite，可以参看这里）提供的渲染函数进行渲染
				letters[i]->RenderEx(fx, y, fRot, fScale*fProportion, fScale);
				// 渲染之后继续更新fx坐标，以正确渲染下一字符
				// 注意计算公式，为 （字宽+后位移+间距）*缩放*宽比

				fx += (letters[i]->GetWidth()+post[i]+fTracking)*fScale*fProportion;
			}
		}
		// 更新至下一个字符的处理
		string++;
	}
}

void hgeFont::printf(float x, float y, int align, const char *format, ...)
{
	// 获取可变参数的起始位置
	char	*pArg=(char *) &format+sizeof(format);
	// 使用_vsnprintf将格式化字符串打印至buffer中
	_vsnprintf(buffer, sizeof(buffer)-1, format, pArg);
	// 将字符串最后一位置空
	buffer[sizeof(buffer)-1]=0;
	//vsprintf(buffer, format, pArg);
	// 调用自身的Render函数进行渲染
	Render(x,y,align,buffer);
}

void hgeFont::printfb(float x, float y, float w, float h, int align, const char *format, ...)
{
	char	chr, *pbuf, *prevword, *linestart;
	int		i,lines=0;
	float	tx, ty, hh, ww;
	// 取得可变参数起始位置
	char	*pArg=(char *) &format+sizeof(format);
	// 使用_vsnprintf将格式化字符串打印至buffer中
	_vsnprintf(buffer, sizeof(buffer)-1, format, pArg);
	buffer[sizeof(buffer)-1]=0;
	//vsprintf(buffer, format, pArg);

	linestart=buffer;
	pbuf=buffer;
	prevword=0;

	for(;;)
	{
		i=0;
		while(pbuf[i] && pbuf[i]!=' ' && pbuf[i]!='\n') i++;

		chr=pbuf[i];
		pbuf[i]=0;
		ww=GetStringWidth(linestart);
		pbuf[i]=chr;

		if(ww > w)
		{
			if(pbuf==linestart)
			{
				pbuf[i]='\n';
				linestart=&pbuf[i+1];
			}
			else
			{
				*prevword='\n';
				linestart=prevword+1;
			}

			lines++;
		}

		if(pbuf[i]=='\n')
		{
			prevword=&pbuf[i];
			linestart=&pbuf[i+1];
			pbuf=&pbuf[i+1];
			lines++;
			continue;
		}

		if(!pbuf[i]) {lines++;break;}

		prevword=&pbuf[i];
		pbuf=&pbuf[i+1];
	}
	
	tx=x;
	ty=y;
	hh=fHeight*fSpacing*fScale*lines;

	switch(align & HGETEXT_HORZMASK)
	{
		case HGETEXT_LEFT: break;
		case HGETEXT_RIGHT: tx+=w; break;
		case HGETEXT_CENTER: tx+=int(w/2); break;
	}

	switch(align & HGETEXT_VERTMASK)
	{
		case HGETEXT_TOP: break;
		case HGETEXT_BOTTOM: ty+=h-hh; break;
		case HGETEXT_MIDDLE: ty+=int((h-hh)/2); break;
	}

	Render(tx,ty,align,buffer);
}

float hgeFont::GetStringWidth(const char *string, bool bMultiline) const
{
	int i;
	float linew, w = 0;

	while(*string)
	{
		linew = 0;

		while(*string && *string != '\n')
		{
			i=(unsigned char)*string;
			if(!letters[i]) i='?';
			if(letters[i])
				linew += letters[i]->GetWidth() + pre[i] + post[i] + fTracking;

			string++;
		}

		if(!bMultiline) return linew*fScale*fProportion;

		if(linew > w) w = linew;

		while (*string == '\n' || *string == '\r') string++;
	}

	return w*fScale*fProportion;
}

void hgeFont::SetColor(DWORD col)
{
	dwCol = col;

	for(int i=0; i<256; i++)
		if(letters[i])
			letters[i]->SetColor(col);
}

void hgeFont::SetZ(float z)
{
	fZ = z;

	for(int i=0; i<256; i++)
		if(letters[i])
			letters[i]->SetZ(z);
}

void hgeFont::SetBlendMode(int blend)
{
	nBlend = blend;

	for(int i=0; i<256; i++)
		if(letters[i])
			letters[i]->SetBlendMode(blend);
}

char *hgeFont::_get_line(char *file, char *line)
{
	int i=0;

	if(!file[i]) return 0;

	while(file[i] && file[i]!='\n' && file[i]!='\r')
	{
		line[i]=file[i];
		i++;
	}
	line[i]=0;

	while(file[i] && (file[i]=='\n' || file[i]=='\r')) i++;

	return file + i;
}