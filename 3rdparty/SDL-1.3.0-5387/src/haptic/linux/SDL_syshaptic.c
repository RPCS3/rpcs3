/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2008 Edgar Simo

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifdef SDL_HAPTIC_LINUX

#include "SDL_haptic.h"
#include "../SDL_syshaptic.h"
#include "SDL_joystick.h"
#include "../../joystick/SDL_sysjoystick.h"     /* For the real SDL_Joystick */
#include "../../joystick/linux/SDL_sysjoystick_c.h"     /* For joystick hwdata */

#include <unistd.h>             /* close */
#include <linux/input.h>        /* Force feedback linux stuff. */
#include <fcntl.h>              /* O_RDWR */
#include <limits.h>             /* INT_MAX */
#include <errno.h>              /* errno, strerror */
#include <math.h>               /* atan2 */
#include <sys/stat.h>           /* stat */

/* Just in case. */
#ifndef M_PI
#  define M_PI     3.14159265358979323846
#endif


#define MAX_HAPTICS  32         /* It's doubtful someone has more then 32 evdev */


/*
 * List of available haptic devices.
 */
static struct
{
    char *fname;                /* Dev path name (like /dev/input/event1) */
    SDL_Haptic *haptic;         /* Assosciated haptic. */
} SDL_hapticlist[MAX_HAPTICS];


/*
 * Haptic system hardware data.
 */
struct haptic_hwdata
{
    int fd;                     /* File descriptor of the device. */
    char *fname;                /* Points to the name in SDL_hapticlist. */
};


/*
 * Haptic system effect data.
 */
struct haptic_hweffect
{
    struct ff_effect effect;    /* The linux kernel effect structure. */
};



#define test_bit(nr, addr) \
   (((1UL << ((nr) & 31)) & (((const unsigned int *) addr)[(nr) >> 5])) != 0)
#define EV_TEST(ev,f) \
   if (test_bit((ev), features)) ret |= (f);
/*
 * Test whether a device has haptic properties.
 * Returns available properties or 0 if there are none.
 */
static int
EV_IsHaptic(int fd)
{
    unsigned int ret;
    unsigned long features[1 + FF_MAX / sizeof(unsigned long)];

    /* Ask device for what it has. */
    ret = 0;
    if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof(features)), features) < 0) {
        SDL_SetError("Haptic: Unable to get device's features: %s",
                     strerror(errno));
        return -1;
    }

    /* Convert supported features to SDL_HAPTIC platform-neutral features. */
    EV_TEST(FF_CONSTANT, SDL_HAPTIC_CONSTANT);
    EV_TEST(FF_SINE, SDL_HAPTIC_SINE);
    EV_TEST(FF_SQUARE, SDL_HAPTIC_SQUARE);
    EV_TEST(FF_TRIANGLE, SDL_HAPTIC_TRIANGLE);
    EV_TEST(FF_SAW_UP, SDL_HAPTIC_SAWTOOTHUP);
    EV_TEST(FF_SAW_DOWN, SDL_HAPTIC_SAWTOOTHDOWN);
    EV_TEST(FF_RAMP, SDL_HAPTIC_RAMP);
    EV_TEST(FF_SPRING, SDL_HAPTIC_SPRING);
    EV_TEST(FF_FRICTION, SDL_HAPTIC_FRICTION);
    EV_TEST(FF_DAMPER, SDL_HAPTIC_DAMPER);
    EV_TEST(FF_INERTIA, SDL_HAPTIC_INERTIA);
    EV_TEST(FF_CUSTOM, SDL_HAPTIC_CUSTOM);
    EV_TEST(FF_GAIN, SDL_HAPTIC_GAIN);
    EV_TEST(FF_AUTOCENTER, SDL_HAPTIC_AUTOCENTER);

    /* Return what it supports. */
    return ret;
}


/*
 * Tests whether a device is a mouse or not.
 */
static int
EV_IsMouse(int fd)
{
    unsigned long argp[40];

    /* Ask for supported features. */
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(argp)), argp) < 0) {
        return -1;
    }

    /* Currently we only test for BTN_MOUSE which can give fake positives. */
    if (test_bit(BTN_MOUSE, argp) != 0) {
        return 1;
    }

    return 0;
}

/*
 * Initializes the haptic subsystem by finding available devices.
 */
int
SDL_SYS_HapticInit(void)
{
    const char joydev_pattern[] = "/dev/input/event%d";
    dev_t dev_nums[MAX_HAPTICS];
    char path[PATH_MAX];
    struct stat sb;
    int fd;
    int i, j, k;
    int duplicate;
    int numhaptics;

    numhaptics = 0;

    /* 
     * Limit amount of checks to MAX_HAPTICS since we may or may not have
     * permission to some or all devices.
     */
    i = 0;
    for (j = 0; j < MAX_HAPTICS; ++j) {

        snprintf(path, PATH_MAX, joydev_pattern, i++);

        /* check to see if file exists */
        if (stat(path, &sb) != 0)
            break;

        /* check for duplicates */
        duplicate = 0;
        for (k = 0; (k < numhaptics) && !duplicate; ++k) {
            if (sb.st_rdev == dev_nums[k]) {
                duplicate = 1;
            }
        }
        if (duplicate) {
            continue;
        }

        /* try to open */
        fd = open(path, O_RDWR, 0);
        if (fd < 0)
            continue;

#ifdef DEBUG_INPUT_EVENTS
        printf("Checking %s\n", path);
#endif

        /* see if it works */
        if (EV_IsHaptic(fd) > 0) {
            SDL_hapticlist[numhaptics].fname = SDL_strdup(path);
            SDL_hapticlist[numhaptics].haptic = NULL;
            dev_nums[numhaptics] = sb.st_rdev;
            ++numhaptics;
        }
        close(fd);
    }

    return numhaptics;
}


/*
 * Gets the name from a file descriptor.
 */
static const char *
SDL_SYS_HapticNameFromFD(int fd)
{
    static char namebuf[128];

    /* We use the evdev name ioctl. */
    if (ioctl(fd, EVIOCGNAME(sizeof(namebuf)), namebuf) <= 0) {
        return NULL;
    }

    return namebuf;
}


/*
 * Return the name of a haptic device, does not need to be opened.
 */
const char *
SDL_SYS_HapticName(int index)
{
    int fd;
    const char *name;

    /* Open the haptic device. */
    name = NULL;
    fd = open(SDL_hapticlist[index].fname, O_RDONLY, 0);

    if (fd >= 0) {

        name = SDL_SYS_HapticNameFromFD(fd);
        if (name == NULL) {
            /* No name found, return device character device */
            name = SDL_hapticlist[index].fname;
        }
    }
    close(fd);

    return name;
}


/*
 * Opens the haptic device from the file descriptor.
 */
static int
SDL_SYS_HapticOpenFromFD(SDL_Haptic * haptic, int fd)
{
    /* Allocate the hwdata */
    haptic->hwdata = (struct haptic_hwdata *)
        SDL_malloc(sizeof(*haptic->hwdata));
    if (haptic->hwdata == NULL) {
        SDL_OutOfMemory();
        goto open_err;
    }
    SDL_memset(haptic->hwdata, 0, sizeof(*haptic->hwdata));

    /* Set the data. */
    haptic->hwdata->fd = fd;
    haptic->supported = EV_IsHaptic(fd);
    haptic->naxes = 2;          /* Hardcoded for now, not sure if it's possible to find out. */

    /* Set the effects */
    if (ioctl(fd, EVIOCGEFFECTS, &haptic->neffects) < 0) {
        SDL_SetError("Haptic: Unable to query device memory: %s",
                     strerror(errno));
        goto open_err;
    }
    haptic->nplaying = haptic->neffects;        /* Linux makes no distinction. */
    haptic->effects = (struct haptic_effect *)
        SDL_malloc(sizeof(struct haptic_effect) * haptic->neffects);
    if (haptic->effects == NULL) {
        SDL_OutOfMemory();
        goto open_err;
    }
    /* Clear the memory */
    SDL_memset(haptic->effects, 0,
               sizeof(struct haptic_effect) * haptic->neffects);

    return 0;

    /* Error handling */
  open_err:
    close(fd);
    if (haptic->hwdata != NULL) {
        free(haptic->hwdata);
        haptic->hwdata = NULL;
    }
    return -1;
}


/*
 * Opens a haptic device for usage.
 */
int
SDL_SYS_HapticOpen(SDL_Haptic * haptic)
{
    int fd;
    int ret;

    /* Open the character device */
    fd = open(SDL_hapticlist[haptic->index].fname, O_RDWR, 0);
    if (fd < 0) {
        SDL_SetError("Haptic: Unable to open %s: %s",
                     SDL_hapticlist[haptic->index], strerror(errno));
        return -1;
    }

    /* Try to create the haptic. */
    ret = SDL_SYS_HapticOpenFromFD(haptic, fd); /* Already closes on error. */
    if (ret < 0) {
        return -1;
    }

    /* Set the fname. */
    haptic->hwdata->fname = SDL_hapticlist[haptic->index].fname;
    return 0;
}


/*
 * Opens a haptic device from first mouse it finds for usage.
 */
int
SDL_SYS_HapticMouse(void)
{
    int fd;
    int i;

    for (i = 0; i < SDL_numhaptics; i++) {

        /* Open the device. */
        fd = open(SDL_hapticlist[i].fname, O_RDWR, 0);
        if (fd < 0) {
            SDL_SetError("Haptic: Unable to open %s: %s",
                         SDL_hapticlist[i], strerror(errno));
            return -1;
        }

        /* Is it a mouse? */
        if (EV_IsMouse(fd)) {
            close(fd);
            return i;
        }

        close(fd);
    }

    return -1;
}


/*
 * Checks to see if a joystick has haptic features.
 */
int
SDL_SYS_JoystickIsHaptic(SDL_Joystick * joystick)
{
    return EV_IsHaptic(joystick->hwdata->fd);
}


/*
 * Checks to see if the haptic device and joystick and in reality the same.
 */
int
SDL_SYS_JoystickSameHaptic(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    /* We are assuming linux is using evdev which should trump the old
     * joystick methods. */
    if (SDL_strcmp(joystick->hwdata->fname, haptic->hwdata->fname) == 0) {
        return 1;
    }
    return 0;
}


/*
 * Opens a SDL_Haptic from a SDL_Joystick.
 */
int
SDL_SYS_HapticOpenFromJoystick(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    int i;
    int fd;
    int ret;


    /* Find the joystick in the haptic list. */
    for (i = 0; i < MAX_HAPTICS; i++) {
        if (SDL_hapticlist[i].fname != NULL) {
            if (SDL_strcmp(SDL_hapticlist[i].fname, joystick->hwdata->fname)
                == 0) {
                haptic->index = i;
                break;
            }
        }
    }
    if (i >= MAX_HAPTICS) {
        SDL_SetError("Haptic: Joystick doesn't have Haptic capabilities");
        return -1;
    }

    fd = open(joystick->hwdata->fname, O_RDWR, 0);
    if (fd < 0) {
        SDL_SetError("Haptic: Unable to open %s: %s",
                     joystick->hwdata->fname, strerror(errno));
        return -1;
    }
    ret = SDL_SYS_HapticOpenFromFD(haptic, fd); /* Already closes on error. */
    if (ret < 0) {
        return -1;
    }

    haptic->hwdata->fname = SDL_hapticlist[haptic->index].fname;
    return 0;
}


/*
 * Closes the haptic device.
 */
void
SDL_SYS_HapticClose(SDL_Haptic * haptic)
{
    if (haptic->hwdata) {

        /* Free effects. */
        SDL_free(haptic->effects);
        haptic->effects = NULL;
        haptic->neffects = 0;

        /* Clean up */
        close(haptic->hwdata->fd);

        /* Free */
        SDL_free(haptic->hwdata);
        haptic->hwdata = NULL;
    }

    /* Clear the rest. */
    SDL_memset(haptic, 0, sizeof(SDL_Haptic));
}


/* 
 * Clean up after system specific haptic stuff
 */
void
SDL_SYS_HapticQuit(void)
{
    int i;

    for (i = 0; SDL_hapticlist[i].fname != NULL; i++) {
        /* Opened and not closed haptics are leaked, this is on purpose.
         * Close your haptic devices after usage. */

        SDL_free(SDL_hapticlist[i].fname);
    }
    SDL_hapticlist[0].fname = NULL;
}


/*
 * Converts an SDL button to a ff_trigger button.
 */
static Uint16
SDL_SYS_ToButton(Uint16 button)
{
    Uint16 ff_button;

    ff_button = 0;

    /*
     * Not sure what the proper syntax is because this actually isn't implemented
     * in the current kernel from what I've seen (2.6.26).
     */
    if (button != 0) {
        ff_button = BTN_GAMEPAD + button - 1;
    }

    return ff_button;
}


/*
 * Returns the ff_effect usable direction from a SDL_HapticDirection.
 */
static Uint16
SDL_SYS_ToDirection(SDL_HapticDirection * dir)
{
    Uint32 tmp;
    float f;                    /* Ideally we'd use fixed point math instead of floats... */

    switch (dir->type) {
    case SDL_HAPTIC_POLAR:
        /* Linux directions start from south.
        		(and range from 0 to 0xFFFF)
				   Quoting include/linux/input.h, line 926:
				   Direction of the effect is encoded as follows:
						0 deg -> 0x0000 (down)
						90 deg -> 0x4000 (left)
						180 deg -> 0x8000 (up)
						270 deg -> 0xC000 (right)
					*/
        tmp = (((18000 + dir->dir[0]) % 36000) * 0xFFFF) / 36000; // convert to range [0,0xFFFF]
        return (Uint16) tmp;

	   case SDL_HAPTIC_SPHERICAL:
   			/*
   				We convert to polar, because that's the only supported direction on Linux.
   				The first value of a spherical direction is practically the same as a
   				Polar direction, except that we have to add 90 degrees. It is the angle
   				from EAST {1,0} towards SOUTH {0,1}.
   				--> add 9000
   				--> finally add 18000 and convert to [0,0xFFFF] as in case SDL_HAPTIC_POLAR.
   			*/
		   	tmp = ((dir->dir[0]) + 9000) % 36000;    /* Convert to polars */
        tmp = (((18000 + tmp) % 36000) * 0xFFFF) / 36000; // convert to range [0,0xFFFF]
        return (Uint16) tmp;

    case SDL_HAPTIC_CARTESIAN:
        f = atan2(dir->dir[1], dir->dir[0]);
					/* 
					  atan2 takes the parameters: Y-axis-value and X-axis-value (in that order)
					   - Y-axis-value is the second coordinate (from center to SOUTH)
					   - X-axis-value is the first coordinate (from center to EAST)
						We add 36000, because atan2 also returns negative values. Then we practically
						have the first spherical value. Therefore we proceed as in case
						SDL_HAPTIC_SPHERICAL and add another 9000 to get the polar value.
					  --> add 45000 in total
					  --> finally add 18000 and convert to [0,0xFFFF] as in case SDL_HAPTIC_POLAR.
					*/
				tmp = (((int) (f * 18000. / M_PI)) + 45000) % 36000;
        tmp = (((18000 + tmp) % 36000) * 0xFFFF) / 36000; // convert to range [0,0xFFFF]
        return (Uint16) tmp;

    default:
        SDL_SetError("Haptic: Unsupported direction type.");
        return (Uint16) - 1;
    }

    return 0;
}


#define  CLAMP(x)    (((x) > 32767) ? 32767 : x)
/*
 * Initializes the linux effect struct from a haptic_effect.
 * Values above 32767 (for unsigned) are unspecified so we must clamp.
 */
static int
SDL_SYS_ToFFEffect(struct ff_effect *dest, SDL_HapticEffect * src)
{
    Uint32 tmp;
    SDL_HapticConstant *constant;
    SDL_HapticPeriodic *periodic;
    SDL_HapticCondition *condition;
    SDL_HapticRamp *ramp;

    /* Clear up */
    SDL_memset(dest, 0, sizeof(struct ff_effect));

    switch (src->type) {
    case SDL_HAPTIC_CONSTANT:
        constant = &src->constant;

        /* Header */
        dest->type = FF_CONSTANT;
        dest->direction = SDL_SYS_ToDirection(&constant->direction);
        if (dest->direction == (Uint16) - 1)
            return -1;

        /* Replay */
        dest->replay.length = (constant->length == SDL_HAPTIC_INFINITY) ?
            0 : CLAMP(constant->length);
        dest->replay.delay = CLAMP(constant->delay);

        /* Trigger */
        dest->trigger.button = SDL_SYS_ToButton(constant->button);
        dest->trigger.interval = CLAMP(constant->interval);

        /* Constant */
        dest->u.constant.level = constant->level;

        /* Envelope */
        dest->u.constant.envelope.attack_length =
            CLAMP(constant->attack_length);
        dest->u.constant.envelope.attack_level =
            CLAMP(constant->attack_level);
        dest->u.constant.envelope.fade_length = CLAMP(constant->fade_length);
        dest->u.constant.envelope.fade_level = CLAMP(constant->fade_level);

        break;

    case SDL_HAPTIC_SINE:
    case SDL_HAPTIC_SQUARE:
    case SDL_HAPTIC_TRIANGLE:
    case SDL_HAPTIC_SAWTOOTHUP:
    case SDL_HAPTIC_SAWTOOTHDOWN:
        periodic = &src->periodic;

        /* Header */
        dest->type = FF_PERIODIC;
        dest->direction = SDL_SYS_ToDirection(&periodic->direction);
        if (dest->direction == (Uint16) - 1)
            return -1;

        /* Replay */
        dest->replay.length = (periodic->length == SDL_HAPTIC_INFINITY) ?
            0 : CLAMP(periodic->length);
        dest->replay.delay = CLAMP(periodic->delay);

        /* Trigger */
        dest->trigger.button = SDL_SYS_ToButton(periodic->button);
        dest->trigger.interval = CLAMP(periodic->interval);

        /* Periodic */
        if (periodic->type == SDL_HAPTIC_SINE)
            dest->u.periodic.waveform = FF_SINE;
        else if (periodic->type == SDL_HAPTIC_SQUARE)
            dest->u.periodic.waveform = FF_SQUARE;
        else if (periodic->type == SDL_HAPTIC_TRIANGLE)
            dest->u.periodic.waveform = FF_TRIANGLE;
        else if (periodic->type == SDL_HAPTIC_SAWTOOTHUP)
            dest->u.periodic.waveform = FF_SAW_UP;
        else if (periodic->type == SDL_HAPTIC_SAWTOOTHDOWN)
            dest->u.periodic.waveform = FF_SAW_DOWN;
        dest->u.periodic.period = CLAMP(periodic->period);
        dest->u.periodic.magnitude = periodic->magnitude;
        dest->u.periodic.offset = periodic->offset;
        /* Phase is calculated based of offset from period and then clamped. */
        tmp = ((periodic->phase % 36000) * dest->u.periodic.period) / 36000;
        dest->u.periodic.phase = CLAMP(tmp);

        /* Envelope */
        dest->u.periodic.envelope.attack_length =
            CLAMP(periodic->attack_length);
        dest->u.periodic.envelope.attack_level =
            CLAMP(periodic->attack_level);
        dest->u.periodic.envelope.fade_length = CLAMP(periodic->fade_length);
        dest->u.periodic.envelope.fade_level = CLAMP(periodic->fade_level);

        break;

    case SDL_HAPTIC_SPRING:
    case SDL_HAPTIC_DAMPER:
    case SDL_HAPTIC_INERTIA:
    case SDL_HAPTIC_FRICTION:
        condition = &src->condition;

        /* Header */
        if (condition->type == SDL_HAPTIC_SPRING)
            dest->type = FF_SPRING;
        else if (condition->type == SDL_HAPTIC_DAMPER)
            dest->type = FF_DAMPER;
        else if (condition->type == SDL_HAPTIC_INERTIA)
            dest->type = FF_INERTIA;
        else if (condition->type == SDL_HAPTIC_FRICTION)
            dest->type = FF_FRICTION;
        dest->direction = 0;    /* Handled by the condition-specifics. */

        /* Replay */
        dest->replay.length = (condition->length == SDL_HAPTIC_INFINITY) ?
            0 : CLAMP(condition->length);
        dest->replay.delay = CLAMP(condition->delay);

        /* Trigger */
        dest->trigger.button = SDL_SYS_ToButton(condition->button);
        dest->trigger.interval = CLAMP(condition->interval);

        /* Condition */
        /* X axis */
        dest->u.condition[0].right_saturation =
            CLAMP(condition->right_sat[0]);
        dest->u.condition[0].left_saturation = CLAMP(condition->left_sat[0]);
        dest->u.condition[0].right_coeff = condition->right_coeff[0];
        dest->u.condition[0].left_coeff = condition->left_coeff[0];
        dest->u.condition[0].deadband = CLAMP(condition->deadband[0]);
        dest->u.condition[0].center = condition->center[0];
        /* Y axis */
        dest->u.condition[1].right_saturation =
            CLAMP(condition->right_sat[1]);
        dest->u.condition[1].left_saturation = CLAMP(condition->left_sat[1]);
        dest->u.condition[1].right_coeff = condition->right_coeff[1];
        dest->u.condition[1].left_coeff = condition->left_coeff[1];
        dest->u.condition[1].deadband = CLAMP(condition->deadband[1]);
        dest->u.condition[1].center = condition->center[1];

        /*
         * There is no envelope in the linux force feedback api for conditions.
         */

        break;

    case SDL_HAPTIC_RAMP:
        ramp = &src->ramp;

        /* Header */
        dest->type = FF_RAMP;
        dest->direction = SDL_SYS_ToDirection(&ramp->direction);
        if (dest->direction == (Uint16) - 1)
            return -1;

        /* Replay */
        dest->replay.length = (ramp->length == SDL_HAPTIC_INFINITY) ?
            0 : CLAMP(ramp->length);
        dest->replay.delay = CLAMP(ramp->delay);

        /* Trigger */
        dest->trigger.button = SDL_SYS_ToButton(ramp->button);
        dest->trigger.interval = CLAMP(ramp->interval);

        /* Ramp */
        dest->u.ramp.start_level = ramp->start;
        dest->u.ramp.end_level = ramp->end;

        /* Envelope */
        dest->u.ramp.envelope.attack_length = CLAMP(ramp->attack_length);
        dest->u.ramp.envelope.attack_level = CLAMP(ramp->attack_level);
        dest->u.ramp.envelope.fade_length = CLAMP(ramp->fade_length);
        dest->u.ramp.envelope.fade_level = CLAMP(ramp->fade_level);

        break;


    default:
        SDL_SetError("Haptic: Unknown effect type.");
        return -1;
    }

    return 0;
}


/*
 * Creates a new haptic effect.
 */
int
SDL_SYS_HapticNewEffect(SDL_Haptic * haptic, struct haptic_effect *effect,
                        SDL_HapticEffect * base)
{
    struct ff_effect *linux_effect;

    /* Allocate the hardware effect */
    effect->hweffect = (struct haptic_hweffect *)
        SDL_malloc(sizeof(struct haptic_hweffect));
    if (effect->hweffect == NULL) {
        SDL_OutOfMemory();
        return -1;
    }

    /* Prepare the ff_effect */
    linux_effect = &effect->hweffect->effect;
    if (SDL_SYS_ToFFEffect(linux_effect, base) != 0) {
        goto new_effect_err;
    }
    linux_effect->id = -1;      /* Have the kernel give it an id */

    /* Upload the effect */
    if (ioctl(haptic->hwdata->fd, EVIOCSFF, linux_effect) < 0) {
        SDL_SetError("Haptic: Error uploading effect to the device: %s",
                     strerror(errno));
        goto new_effect_err;
    }

    return 0;

  new_effect_err:
    free(effect->hweffect);
    effect->hweffect = NULL;
    return -1;
}


/*
 * Updates an effect.
 *
 * Note: Dynamically updating the direction can in some cases force
 * the effect to restart and run once.
 */
int
SDL_SYS_HapticUpdateEffect(SDL_Haptic * haptic,
                           struct haptic_effect *effect,
                           SDL_HapticEffect * data)
{
    struct ff_effect linux_effect;

    /* Create the new effect */
    if (SDL_SYS_ToFFEffect(&linux_effect, data) != 0) {
        return -1;
    }
    linux_effect.id = effect->hweffect->effect.id;

    /* See if it can be uploaded. */
    if (ioctl(haptic->hwdata->fd, EVIOCSFF, &linux_effect) < 0) {
        SDL_SetError("Haptic: Error updating the effect: %s",
                     strerror(errno));
        return -1;
    }

    /* Copy the new effect into memory. */
    SDL_memcpy(&effect->hweffect->effect, &linux_effect,
               sizeof(struct ff_effect));

    return effect->hweffect->effect.id;
}


/*
 * Runs an effect.
 */
int
SDL_SYS_HapticRunEffect(SDL_Haptic * haptic, struct haptic_effect *effect,
                        Uint32 iterations)
{
    struct input_event run;

    /* Prepare to run the effect */
    run.type = EV_FF;
    run.code = effect->hweffect->effect.id;
    /* We don't actually have infinity here, so we just do INT_MAX which is pretty damn close. */
    run.value = (iterations > INT_MAX) ? INT_MAX : iterations;

    if (write(haptic->hwdata->fd, (const void *) &run, sizeof(run)) < 0) {
        SDL_SetError("Haptic: Unable to run the effect: %s", strerror(errno));
        return -1;
    }

    return 0;
}


/*
 * Stops an effect.
 */
int
SDL_SYS_HapticStopEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    struct input_event stop;

    stop.type = EV_FF;
    stop.code = effect->hweffect->effect.id;
    stop.value = 0;

    if (write(haptic->hwdata->fd, (const void *) &stop, sizeof(stop)) < 0) {
        SDL_SetError("Haptic: Unable to stop the effect: %s",
                     strerror(errno));
        return -1;
    }

    return 0;
}


/*
 * Frees the effect.
 */
void
SDL_SYS_HapticDestroyEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    if (ioctl(haptic->hwdata->fd, EVIOCRMFF, effect->hweffect->effect.id) < 0) {
        SDL_SetError("Haptic: Error removing the effect from the device: %s",
                     strerror(errno));
    }
    SDL_free(effect->hweffect);
    effect->hweffect = NULL;
}


/*
 * Gets the status of a haptic effect.
 */
int
SDL_SYS_HapticGetEffectStatus(SDL_Haptic * haptic,
                              struct haptic_effect *effect)
{
#if 0                           /* Not supported atm. */
    struct input_event ie;

    ie.type = EV_FF;
    ie.type = EV_FF_STATUS;
    ie.code = effect->hweffect->effect.id;

    if (write(haptic->hwdata->fd, &ie, sizeof(ie)) < 0) {
        SDL_SetError("Haptic: Error getting device status.");
        return -1;
    }

    return 0;
#endif

    return -1;
}


/*
 * Sets the gain.
 */
int
SDL_SYS_HapticSetGain(SDL_Haptic * haptic, int gain)
{
    struct input_event ie;

    ie.type = EV_FF;
    ie.code = FF_GAIN;
    ie.value = (0xFFFFUL * gain) / 100;

    if (write(haptic->hwdata->fd, &ie, sizeof(ie)) < 0) {
        SDL_SetError("Haptic: Error setting gain: %s", strerror(errno));
        return -1;
    }

    return 0;
}


/*
 * Sets the autocentering.
 */
int
SDL_SYS_HapticSetAutocenter(SDL_Haptic * haptic, int autocenter)
{
    struct input_event ie;

    ie.type = EV_FF;
    ie.code = FF_AUTOCENTER;
    ie.value = (0xFFFFUL * autocenter) / 100;

    if (write(haptic->hwdata->fd, &ie, sizeof(ie)) < 0) {
        SDL_SetError("Haptic: Error setting autocenter: %s", strerror(errno));
        return -1;
    }

    return 0;
}


/*
 * Pausing is not supported atm by linux.
 */
int
SDL_SYS_HapticPause(SDL_Haptic * haptic)
{
    return -1;
}


/*
 * Unpausing is not supported atm by linux.
 */
int
SDL_SYS_HapticUnpause(SDL_Haptic * haptic)
{
    return -1;
}


/*
 * Stops all the currently playing effects.
 */
int
SDL_SYS_HapticStopAll(SDL_Haptic * haptic)
{
    int i, ret;

    /* Linux does not support this natively so we have to loop. */
    for (i = 0; i < haptic->neffects; i++) {
        if (haptic->effects[i].hweffect != NULL) {
            ret = SDL_SYS_HapticStopEffect(haptic, &haptic->effects[i]);
            if (ret < 0) {
                SDL_SetError
                    ("Haptic: Error while trying to stop all playing effects.");
                return -1;
            }
        }
    }
    return 0;
}


#endif /* SDL_HAPTIC_LINUX */
