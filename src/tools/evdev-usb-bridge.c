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
#define KEYCODE_WIPERS 0x1a // w
#define KEYCODE_HUD 0x19 // v

int main(int argc, char* argv[])
{
    int ret = 0;
    int evdev_fd = open(argv[1], O_WRONLY);
    if (!evdev_fd)
	goto evdev_open_fail;

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == -1)
	goto sock_open_fail;

    struct sockaddr_in si_me;
    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(32008);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr*) &si_me, sizeof(si_me)) == -1)
	goto bind_fail;

    struct input_event ev, syn;
    memset(&ev, 0, sizeof(struct input_event));
    ev.type = EV_KEY;
    //filled-out on each UDP packet received

    memset(&syn, 0, sizeof(struct input_event));
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;

    while(1)
    {
	struct sockaddr_in si_other;
	socklen_t slen = sizeof(si_other);
	int rlen;
	char buf[2];

	if ( (rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen)) == -1)
	    goto recv_fail;
	if (rlen != 2)
	    continue;

	if (buf[0] != KEYCODE_WIPERS
	    && buf[0] != KEYCODE_HUD)
	    continue;

	ev.code = buf[0];
	ev.value = buf[1];

	ret = TEMP_FAILURE_RETRY(write(evdev_fd, &ev, sizeof(ev)));
	if (!ret)
	    goto write_fail;

	ret = TEMP_FAILURE_RETRY(write(evdev_fd, &syn, sizeof(syn)));
	if (!ret)
	    goto write_fail;
    }

    goto done;
write_fail:
recv_fail:
bind_fail:
    close(s);
sock_open_fail:
    close(evdev_fd);
evdev_open_fail:
done:
    return ret;
}
