//Scrolling auto populating tiles
while(1) {
		gfxSleepMilliseconds(5);
        gdispGClear(GDISP, (LUMA2COLOR(0)));
        x=x+4;y=y+4;
        for(int tempx = x+4*256; tempx>-256; tempx=tempx-256)
        {
            while(tempx > 4*256)
            {
                tempx=tempx-256;
            }
            
            for(int tempy = y+4*256; tempy>-256; tempy=tempy-256)
            {   
                while(tempy > 4*256)
                {
                    tempy=tempy-256;
                }
                //drawTile(tempx, tempy);
                gdispGDrawBox(GDISP, tempx, tempy, 256, 256, RGB2COLOR(255,128,128));
            }
        }
	}