/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software    Founation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include "SDL_config.h"

/* General mouse handling code for SDL */

#include "SDL_events.h"
#include "SDL_events_c.h"
#include "SDL_gesture_c.h"

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

//TODO: Replace with malloc

#define MAXPATHSIZE 1024

 


#define DOLLARNPOINTS 64
#define DOLLARSIZE 256

#define ENABLE_DOLLAR

#define PHI 0.618033989 

typedef struct {
  float x,y;
} SDL_FloatPoint;

typedef struct {
  float length;
  
  int numPoints;
  SDL_FloatPoint p[MAXPATHSIZE];
} SDL_DollarPath;

typedef struct {
  SDL_FloatPoint path[DOLLARNPOINTS];
  unsigned long hash;
} SDL_DollarTemplate;

typedef struct {
  SDL_GestureID id;
  SDL_FloatPoint res;
  SDL_FloatPoint centroid;
  SDL_DollarPath dollarPath;
  Uint16 numDownFingers;

  int numDollarTemplates;
  SDL_DollarTemplate *dollarTemplate;

  SDL_bool recording;
} SDL_GestureTouch;

SDL_GestureTouch *SDL_gestureTouch;
int SDL_numGestureTouches = 0;
SDL_bool recordAll;

#if 0
static void PrintPath(SDL_FloatPoint *path) {
  int i;
  printf("Path:");
  for(i=0;i<DOLLARNPOINTS;i++) {
    printf(" (%f,%f)",path[i].x,path[i].y);
  }
  printf("\n");
}
#endif

int SDL_RecordGesture(SDL_TouchID touchId) {
  int i;
  if(touchId < 0) recordAll = SDL_TRUE;
  for(i = 0;i < SDL_numGestureTouches; i++) {
    if((touchId < 0) || (SDL_gestureTouch[i].id == touchId)) {
      SDL_gestureTouch[i].recording = SDL_TRUE;
      if(touchId >= 0)
        return 1;
    }      
  }
  return (touchId < 0);
}

unsigned long SDL_HashDollar(SDL_FloatPoint* points) {
  unsigned long hash = 5381;
  int i;
  for(i = 0;i < DOLLARNPOINTS; i++) { 
    hash = ((hash<<5) + hash) + (unsigned long)points[i].x;
    hash = ((hash<<5) + hash) + (unsigned long)points[i].y;
  }
  return hash;
}


static int SaveTemplate(SDL_DollarTemplate *templ, SDL_RWops * src) {
  if(src == NULL) return 0;

  
  //No Longer storing the Hash, rehash on load
  //if(SDL_RWops.write(src,&(templ->hash),sizeof(templ->hash),1) != 1) return 0;
  
  if(SDL_RWwrite(src,templ->path,
                 sizeof(templ->path[0]),DOLLARNPOINTS) != DOLLARNPOINTS) 
    return 0;

  return 1;
}


int SDL_SaveAllDollarTemplates(SDL_RWops *src) {  
  int i,j,rtrn = 0;
  for(i = 0; i < SDL_numGestureTouches; i++) {
    SDL_GestureTouch* touch = &SDL_gestureTouch[i];
    for(j = 0;j < touch->numDollarTemplates; j++) {
        rtrn += SaveTemplate(&touch->dollarTemplate[i],src);
    }
  }
  return rtrn;  
}

int SDL_SaveDollarTemplate(SDL_GestureID gestureId, SDL_RWops *src) {
  int i,j;
  for(i = 0; i < SDL_numGestureTouches; i++) {
    SDL_GestureTouch* touch = &SDL_gestureTouch[i];
    for(j = 0;j < touch->numDollarTemplates; j++) {
      if(touch->dollarTemplate[i].hash == gestureId) {
        return SaveTemplate(&touch->dollarTemplate[i],src);
      }
    }
  }
  SDL_SetError("Unknown gestureId");
  return -1;
}

//path is an already sampled set of points
//Returns the index of the gesture on success, or -1
static int SDL_AddDollarGesture(SDL_GestureTouch* inTouch,SDL_FloatPoint* path) {
  SDL_DollarTemplate* dollarTemplate;
  SDL_DollarTemplate *templ;
  int i = 0;
  if(inTouch == NULL) {
    if(SDL_numGestureTouches == 0) return -1;
    for(i = 0;i < SDL_numGestureTouches; i++) {
      inTouch = &SDL_gestureTouch[i];

    dollarTemplate = 
        (SDL_DollarTemplate *)SDL_realloc(inTouch->dollarTemplate,
                    (inTouch->numDollarTemplates + 1) * 
                    sizeof(SDL_DollarTemplate));
      if(!dollarTemplate) {
        SDL_OutOfMemory();
        return -1;
      }
        
      inTouch->dollarTemplate = dollarTemplate;

    templ = 
        &inTouch->dollarTemplate[inTouch->numDollarTemplates];
      SDL_memcpy(templ->path,path,DOLLARNPOINTS*sizeof(SDL_FloatPoint));
      templ->hash = SDL_HashDollar(templ->path);
      inTouch->numDollarTemplates++;    
    }
    return inTouch->numDollarTemplates - 1;
  } else {
    SDL_DollarTemplate* dollarTemplate = 
      ( SDL_DollarTemplate *)SDL_realloc(inTouch->dollarTemplate,
                  (inTouch->numDollarTemplates + 1) * 
                  sizeof(SDL_DollarTemplate));
    if(!dollarTemplate) {
      SDL_OutOfMemory();
      return -1;
    }
    
    inTouch->dollarTemplate = dollarTemplate;

    templ = 
      &inTouch->dollarTemplate[inTouch->numDollarTemplates];
    SDL_memcpy(templ->path,path,DOLLARNPOINTS*sizeof(SDL_FloatPoint));
    templ->hash = SDL_HashDollar(templ->path);
    inTouch->numDollarTemplates++;
    return inTouch->numDollarTemplates - 1;
  }
  return -1;
}

int SDL_LoadDollarTemplates(SDL_TouchID touchId, SDL_RWops *src) {
  int i,loaded = 0;
  SDL_GestureTouch *touch = NULL;
  if(src == NULL) return 0;
  if(touchId >= 0) {
    for(i = 0;i < SDL_numGestureTouches; i++)
      if(SDL_gestureTouch[i].id == touchId)
        touch = &SDL_gestureTouch[i];
    if(touch == NULL) return -1;
  }

  while(1) {
    SDL_DollarTemplate templ;

    if(SDL_RWread(src,templ.path,sizeof(templ.path[0]),DOLLARNPOINTS) < 
       DOLLARNPOINTS) break;

    if(touchId >= 0) {
      //printf("Adding loaded gesture to 1 touch\n");
      if(SDL_AddDollarGesture(touch,templ.path)) loaded++;
    }
    else {
      //printf("Adding to: %i touches\n",SDL_numGestureTouches);
      for(i = 0;i < SDL_numGestureTouches; i++) {
        touch = &SDL_gestureTouch[i];
        //printf("Adding loaded gesture to + touches\n");
        //TODO: What if this fails?
        SDL_AddDollarGesture(touch,templ.path);        
      }
      loaded++;
    }
  }

  return loaded; 
}


float dollarDifference(SDL_FloatPoint* points,SDL_FloatPoint* templ,float ang) {
  //  SDL_FloatPoint p[DOLLARNPOINTS];
  float dist = 0;
  SDL_FloatPoint p;
  int i;
  for(i = 0; i < DOLLARNPOINTS; i++) {
    p.x = (float)(points[i].x * SDL_cos(ang) - points[i].y * SDL_sin(ang));
    p.y = (float)(points[i].x * SDL_sin(ang) + points[i].y * SDL_cos(ang));
    dist += (float)(SDL_sqrt((p.x-templ[i].x)*(p.x-templ[i].x)+
                 (p.y-templ[i].y)*(p.y-templ[i].y)));
  }
  return dist/DOLLARNPOINTS;
  
}

float bestDollarDifference(SDL_FloatPoint* points,SDL_FloatPoint* templ) {
  //------------BEGIN DOLLAR BLACKBOX----------------//
  //-TRANSLATED DIRECTLY FROM PSUDEO-CODE AVAILABLE AT-//
  //-"http://depts.washington.edu/aimgroup/proj/dollar/"-//
  double ta = -M_PI/4;
  double tb = M_PI/4;
  double dt = M_PI/90;
  float x1 = (float)(PHI*ta + (1-PHI)*tb);
  float f1 = dollarDifference(points,templ,x1);
  float x2 = (float)((1-PHI)*ta + PHI*tb);
  float f2 = dollarDifference(points,templ,x2);
  while(SDL_fabs(ta-tb) > dt) {
    if(f1 < f2) {
      tb = x2;
      x2 = x1;
      f2 = f1;
      x1 = (float)(PHI*ta + (1-PHI)*tb);
      f1 = dollarDifference(points,templ,x1);
    }
    else {
      ta = x1;
      x1 = x2;
      f1 = f2;
      x2 = (float)((1-PHI)*ta + PHI*tb);
      f2 = dollarDifference(points,templ,x2);
    }
  }
  /*
  if(f1 <= f2)
    printf("Min angle (x1): %f\n",x1);
  else if(f1 >  f2)
    printf("Min angle (x2): %f\n",x2);
  */
  return SDL_min(f1,f2);  
}

//DollarPath contains raw points, plus (possibly) the calculated length
int dollarNormalize(const SDL_DollarPath *path,SDL_FloatPoint *points) {
  int i;
  float interval;
  float dist;
  int numPoints = 0;
  SDL_FloatPoint centroid; 
  float xmin,xmax,ymin,ymax;
  float ang;
  float w,h;
  float length = path->length;

  //Calculate length if it hasn't already been done
  if(length <= 0) {
    for(i=1;i<path->numPoints;i++) {
      float dx = path->p[i  ].x - 
                 path->p[i-1].x;
      float dy = path->p[i  ].y - 
                 path->p[i-1].y;
      length += (float)(SDL_sqrt(dx*dx+dy*dy));
    }
  }

  //Resample
  interval = length/(DOLLARNPOINTS - 1);
  dist = interval;

  centroid.x = 0;centroid.y = 0;
  
  //printf("(%f,%f)\n",path->p[path->numPoints-1].x,path->p[path->numPoints-1].y);
  for(i = 1;i < path->numPoints;i++) {
    float d = (float)(SDL_sqrt((path->p[i-1].x-path->p[i].x)*(path->p[i-1].x-path->p[i].x)+
                             (path->p[i-1].y-path->p[i].y)*(path->p[i-1].y-path->p[i].y)));
    //printf("d = %f dist = %f/%f\n",d,dist,interval);
    while(dist + d > interval) {
      points[numPoints].x = path->p[i-1].x + 
        ((interval-dist)/d)*(path->p[i].x-path->p[i-1].x);
      points[numPoints].y = path->p[i-1].y + 
        ((interval-dist)/d)*(path->p[i].y-path->p[i-1].y);
      centroid.x += points[numPoints].x;
      centroid.y += points[numPoints].y;
      numPoints++;

      dist -= interval;
    }
    dist += d;
  }
  if(numPoints < DOLLARNPOINTS-1) {
    SDL_SetError("ERROR: NumPoints = %i\n",numPoints); 
    return 0;
  }
  //copy the last point
  points[DOLLARNPOINTS-1] = path->p[path->numPoints-1];
  numPoints = DOLLARNPOINTS;

  centroid.x /= numPoints;
  centroid.y /= numPoints;
 
  //printf("Centroid (%f,%f)",centroid.x,centroid.y);
  //Rotate Points so point 0 is left of centroid and solve for the bounding box
  xmin = centroid.x;
  xmax = centroid.x;
  ymin = centroid.y;
  ymax = centroid.y;
  
  ang = (float)(SDL_atan2(centroid.y - points[0].y,
                    centroid.x - points[0].x));

  for(i = 0;i<numPoints;i++) {                                               
    float px = points[i].x;
    float py = points[i].y;
    points[i].x = (float)((px - centroid.x)*SDL_cos(ang) - 
                  (py - centroid.y)*SDL_sin(ang) + centroid.x);
    points[i].y = (float)((px - centroid.x)*SDL_sin(ang) + 
                  (py - centroid.y)*SDL_cos(ang) + centroid.y);


    if(points[i].x < xmin) xmin = points[i].x;
    if(points[i].x > xmax) xmax = points[i].x; 
    if(points[i].y < ymin) ymin = points[i].y;
    if(points[i].y > ymax) ymax = points[i].y;
  }

  //Scale points to DOLLARSIZE, and translate to the origin
  w = xmax-xmin;
  h = ymax-ymin;

  for(i=0;i<numPoints;i++) {
    points[i].x = (points[i].x - centroid.x)*DOLLARSIZE/w;
    points[i].y = (points[i].y - centroid.y)*DOLLARSIZE/h;
  }  
  return numPoints;
}

float dollarRecognize(const SDL_DollarPath *path,int *bestTempl,SDL_GestureTouch* touch) {
        
        SDL_FloatPoint points[DOLLARNPOINTS];
        int numPoints = dollarNormalize(path,points);
        int i;
        float bestDiff = 10000;

        //PrintPath(points);
        *bestTempl = -1;
        for(i = 0;i < touch->numDollarTemplates;i++) {
                float diff = bestDollarDifference(points,touch->dollarTemplate[i].path);
                if(diff < bestDiff) {bestDiff = diff; *bestTempl = i;}
        }
        return bestDiff;
}

int SDL_GestureAddTouch(SDL_Touch* touch) {  
  SDL_GestureTouch *gestureTouch = (SDL_GestureTouch *)SDL_realloc(SDL_gestureTouch,
                                               (SDL_numGestureTouches + 1) *
                                               sizeof(SDL_GestureTouch));

  if(!gestureTouch) {
    SDL_OutOfMemory();
    return -1;
  }

  SDL_gestureTouch = gestureTouch;

  SDL_gestureTouch[SDL_numGestureTouches].res.x = touch->xres;
  SDL_gestureTouch[SDL_numGestureTouches].res.y = touch->yres;
  SDL_gestureTouch[SDL_numGestureTouches].numDownFingers = 0;

  SDL_gestureTouch[SDL_numGestureTouches].res.x = touch->xres;
  SDL_gestureTouch[SDL_numGestureTouches].id = touch->id;

  SDL_gestureTouch[SDL_numGestureTouches].numDollarTemplates = 0;

  SDL_gestureTouch[SDL_numGestureTouches].recording = SDL_FALSE;

  SDL_numGestureTouches++;
  return 0;
}

int SDL_GestureRemoveTouch(SDL_TouchID id) {
  int i;
  for (i = 0; i < SDL_numGestureTouches; i++) {
    if (SDL_gestureTouch[i].id == id) {
      SDL_numGestureTouches--;
      SDL_memcpy(&SDL_gestureTouch[i], &SDL_gestureTouch[SDL_numGestureTouches], sizeof(SDL_gestureTouch[i]));
      return 1;
    }
  }
  return -1;
}


SDL_GestureTouch * SDL_GetGestureTouch(SDL_TouchID id) {
  int i;
  for(i = 0;i < SDL_numGestureTouches; i++) {
    //printf("%i ?= %i\n",SDL_gestureTouch[i].id,id);
    if(SDL_gestureTouch[i].id == id) return &SDL_gestureTouch[i];
  }
  return NULL;
}

int SDL_SendGestureMulti(SDL_GestureTouch* touch,float dTheta,float dDist) {
  SDL_Event event;
  event.mgesture.type = SDL_MULTIGESTURE;
  event.mgesture.touchId = touch->id;
  event.mgesture.x = touch->centroid.x;
  event.mgesture.y = touch->centroid.y;
  event.mgesture.dTheta = dTheta;
  event.mgesture.dDist = dDist;  
  event.mgesture.numFingers = touch->numDownFingers;
  return SDL_PushEvent(&event) > 0;
}

int SDL_SendGestureDollar(SDL_GestureTouch* touch,
                          SDL_GestureID gestureId,float error) {
  SDL_Event event;
  event.dgesture.type = SDL_DOLLARGESTURE;
  event.dgesture.touchId = touch->id;
  /*
    //TODO: Add this to give location of gesture?
  event.mgesture.x = touch->centroid.x;
  event.mgesture.y = touch->centroid.y;
  */
  event.dgesture.gestureId = gestureId;
  event.dgesture.error = error;  
  //A finger came up to trigger this event.
  event.dgesture.numFingers = touch->numDownFingers + 1; 
  return SDL_PushEvent(&event) > 0;
}


int SDL_SendDollarRecord(SDL_GestureTouch* touch,SDL_GestureID gestureId) {
  SDL_Event event;
  event.dgesture.type = SDL_DOLLARRECORD;
  event.dgesture.touchId = touch->id;
  event.dgesture.gestureId = gestureId;
  return SDL_PushEvent(&event) > 0;
}


void SDL_GestureProcessEvent(SDL_Event* event)
{
  float x,y; 
  SDL_FloatPoint path[DOLLARNPOINTS];
  int index;
  int i;
  float pathDx, pathDy;
  SDL_FloatPoint lastP;
  SDL_FloatPoint lastCentroid;
  float lDist;
  float Dist;
  float dtheta;
  float dDist;

  if(event->type == SDL_FINGERMOTION || 
     event->type == SDL_FINGERDOWN ||
     event->type == SDL_FINGERUP) {
    SDL_GestureTouch* inTouch = SDL_GetGestureTouch(event->tfinger.touchId);
    
    //Shouldn't be possible
    if(inTouch == NULL) return;
    
    //printf("@ (%i,%i) with res: (%i,%i)\n",(int)event->tfinger.x,
    //           (int)event->tfinger.y,
    //   (int)inTouch->res.x,(int)inTouch->res.y);

    
    x = ((float)event->tfinger.x)/(float)inTouch->res.x;
    y = ((float)event->tfinger.y)/(float)inTouch->res.y;   


    //Finger Up
    if(event->type == SDL_FINGERUP) {
      inTouch->numDownFingers--;
      
#ifdef ENABLE_DOLLAR
      if(inTouch->recording) {
        inTouch->recording = SDL_FALSE;        
        dollarNormalize(&inTouch->dollarPath,path);
        //PrintPath(path);
        if(recordAll) {
          index = SDL_AddDollarGesture(NULL,path);
          for(i = 0;i < SDL_numGestureTouches; i++)
            SDL_gestureTouch[i].recording = SDL_FALSE;
        }
        else {
          index = SDL_AddDollarGesture(inTouch,path);
        }
        
        if(index >= 0) {
          SDL_SendDollarRecord(inTouch,inTouch->dollarTemplate[index].hash);
        }
        else {
          SDL_SendDollarRecord(inTouch,-1);
        }
      }
      else {        
        int bestTempl;
        float error;
        error = dollarRecognize(&inTouch->dollarPath,
                                &bestTempl,inTouch);
        if(bestTempl >= 0){
          //Send Event
          unsigned long gestureId = inTouch->dollarTemplate[bestTempl].hash;
          SDL_SendGestureDollar(inTouch,gestureId,error);
          //printf ("%s\n",);("Dollar error: %f\n",error);
        }
      }
#endif 
      //inTouch->gestureLast[j] = inTouch->gestureLast[inTouch->numDownFingers];
      if(inTouch->numDownFingers > 0) {
        inTouch->centroid.x = (inTouch->centroid.x*(inTouch->numDownFingers+1)-
                               x)/inTouch->numDownFingers;
        inTouch->centroid.y = (inTouch->centroid.y*(inTouch->numDownFingers+1)-
                               y)/inTouch->numDownFingers;
      }
    }
    else if(event->type == SDL_FINGERMOTION) {
      float dx = ((float)event->tfinger.dx)/(float)inTouch->res.x;
      float dy = ((float)event->tfinger.dy)/(float)inTouch->res.y;
      //printf("dx,dy: (%f,%f)\n",dx,dy); 
#ifdef ENABLE_DOLLAR
      SDL_DollarPath* path = &inTouch->dollarPath;
      if(path->numPoints < MAXPATHSIZE) {
        path->p[path->numPoints].x = inTouch->centroid.x;
        path->p[path->numPoints].y = inTouch->centroid.y;
        pathDx = 
          (path->p[path->numPoints].x-path->p[path->numPoints-1].x);
        pathDy = 
          (path->p[path->numPoints].y-path->p[path->numPoints-1].y);
        path->length += (float)SDL_sqrt(pathDx*pathDx + pathDy*pathDy);
        path->numPoints++;
      }
#endif
      lastP.x = x - dx;
      lastP.y = y - dy;
      lastCentroid = inTouch->centroid;
      
      inTouch->centroid.x += dx/inTouch->numDownFingers;
      inTouch->centroid.y += dy/inTouch->numDownFingers;
      //printf("Centrid : (%f,%f)\n",inTouch->centroid.x,inTouch->centroid.y);
      if(inTouch->numDownFingers > 1) {
        SDL_FloatPoint lv; //Vector from centroid to last x,y position
        SDL_FloatPoint v; //Vector from centroid to current x,y position
        //lv = inTouch->gestureLast[j].cv;
        lv.x = lastP.x - lastCentroid.x;
        lv.y = lastP.y - lastCentroid.y;
        lDist = (float)SDL_sqrt(lv.x*lv.x + lv.y*lv.y);
        //printf("lDist = %f\n",lDist);
        v.x = x - inTouch->centroid.x;
        v.y = y - inTouch->centroid.y;
        //inTouch->gestureLast[j].cv = v;
        Dist = (float)SDL_sqrt(v.x*v.x+v.y*v.y);
        // SDL_cos(dTheta) = (v . lv)/(|v| * |lv|)
        
        //Normalize Vectors to simplify angle calculation
        lv.x/=lDist;
        lv.y/=lDist;
        v.x/=Dist;
        v.y/=Dist;
        dtheta = (float)SDL_atan2(lv.x*v.y - lv.y*v.x,lv.x*v.x + lv.y*v.y);
        
        dDist = (Dist - lDist);
        if(lDist == 0) {dDist = 0;dtheta = 0;} //To avoid impossible values
        
        //inTouch->gestureLast[j].dDist = dDist;
        //inTouch->gestureLast[j].dtheta = dtheta;
        
        //printf("dDist = %f, dTheta = %f\n",dDist,dtheta);
        //gdtheta = gdtheta*.9 + dtheta*.1;
        //gdDist  =  gdDist*.9 +  dDist*.1
        //knob.r += dDist/numDownFingers;
        //knob.ang += dtheta;
        //printf("thetaSum = %f, distSum = %f\n",gdtheta,gdDist);
        //printf("id: %i dTheta = %f, dDist = %f\n",j,dtheta,dDist);
        SDL_SendGestureMulti(inTouch,dtheta,dDist);
      }
      else {
        //inTouch->gestureLast[j].dDist = 0;
        //inTouch->gestureLast[j].dtheta = 0;
        //inTouch->gestureLast[j].cv.x = 0;
        //inTouch->gestureLast[j].cv.y = 0;
      }
      //inTouch->gestureLast[j].f.p.x = x;
      //inTouch->gestureLast[j].f.p.y = y;
      //break;
      //pressure?
    }
    
    if(event->type == SDL_FINGERDOWN) {

      inTouch->numDownFingers++;
      inTouch->centroid.x = (inTouch->centroid.x*(inTouch->numDownFingers - 1)+ 
                             x)/inTouch->numDownFingers;
      inTouch->centroid.y = (inTouch->centroid.y*(inTouch->numDownFingers - 1)+
                             y)/inTouch->numDownFingers;
      //printf("Finger Down: (%f,%f). Centroid: (%f,%f\n",x,y,
      //     inTouch->centroid.x,inTouch->centroid.y);

#ifdef ENABLE_DOLLAR
      inTouch->dollarPath.length = 0;
      inTouch->dollarPath.p[0].x = x;
      inTouch->dollarPath.p[0].y = y;
      inTouch->dollarPath.numPoints = 1;
#endif
    }
  }
}

  /* vi: set ts=4 sw=4 expandtab: */
  
