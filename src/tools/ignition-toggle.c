#define _GNU_SOURCE 1 //for TEMP_FAILURE_RETRY

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

//US layout USB HID Keyboard scan codes -- c.f. http://www.mindrunway.ru/IgorPlHex/USBKeyScan.pdf
#define KEYCODE_IGN   0x0C // i
#define KEYCODE_START 0x16 // s

int main(int argc, char* argv[])
{
    int ret = 0;
    int evdev_fd = open(argv[1], O_WRONLY);
    if (!evdev_fd)
	goto evdev_open_fail;

    struct input_event ev, syn;
    memset(&ev, 0, sizeof(struct input_event));
    ev.type = EV_KEY;

    memset(&syn, 0, sizeof(struct input_event));
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;

    //each time this is executed, send two keypresses: i then s
    ev.code = KEYCODE_IGN;

    ev.value = 1;
    ret = TEMP_FAILURE_RETRY(write(evdev_fd, &ev, sizeof(ev)));
    if (!ret)
	goto write_fail;

    ev.value = 0;
    ret = TEMP_FAILURE_RETRY(write(evdev_fd, &ev, sizeof(ev)));
    if (!ret)
	goto write_fail;

    ev.code = KEYCODE_START;

    ev.value = 1;
    ret = TEMP_FAILURE_RETRY(write(evdev_fd, &ev, sizeof(ev)));
    if (!ret)
	goto write_fail;

    ev.value = 0;
    ret = TEMP_FAILURE_RETRY(write(evdev_fd, &ev, sizeof(ev)));
    if (!ret)
	goto write_fail;

    ret = TEMP_FAILURE_RETRY(write(evdev_fd, &syn, sizeof(syn)));
    if (!ret)
	goto write_fail;

    goto done;

write_fail:
    fprintf(stderr, "failed to write to evdev: %d\n", ret);
    close(evdev_fd);
evdev_open_fail:
done:
    return ret;
}
