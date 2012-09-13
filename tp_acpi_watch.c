/**
 * Reader for ThinkPad ACPI
 */

#define TP_EVENT_NONE 0
#define TP_EVENT_TABUP 1
#define TP_EVENT_TABDOWN 2
#define TP_EVENT_EOF 3

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

struct tp_acpi_event {
	uint64_t	pad1;
	uint64_t	pad2;
	uint16_t	type;
	uint16_t	code;
	uint32_t	value;
} ((packed));

int scan_event(int fd)
{
	struct tp_acpi_event evt;
	size_t nread;
	if ( (nread = read(fd, &evt, sizeof(struct tp_acpi_event))) == sizeof(struct tp_acpi_event) )
	{
		/* printf("read type 0x%x, code 0x%x, value 0x%x\n", evt.type, evt.code, evt.value); */
		if ( evt.type == 5 && evt.code == 1 )
		{
			if ( evt.value == 1 )
				return TP_EVENT_TABDOWN;
			else if ( evt.value == 0 )
				return TP_EVENT_TABUP;
		}
		return TP_EVENT_NONE;
	}
	return TP_EVENT_EOF;
}

void tablet_down()
{
	system("$HOME/bin/rotate right");
	int fd = -1;
	if ( (fd = open("/home/dan/.local/state/tablet", O_WRONLY|O_CREAT|O_TRUNC)) >= 0 )
	{
		write(fd, "down", 4);
		close(fd);
	}
}

void tablet_up()
{
	system("$HOME/bin/rotate normal");
	int fd = -1;
	if ( (fd = open("/home/dan/.local/state/tablet", O_WRONLY|O_CREAT|O_TRUNC)) >= 0 )
	{
		write(fd, "up", 2);
		close(fd);
	}
}

char *find_trackpoint()
{
	FILE *pp = popen("find /sys/devices/platform/i8042 -name name | xargs grep -Fl TrackPoint | sed 's/\\/input\\/input[0-9]*\\/name$//'", "r");
	char *buf = malloc(1024);
	while ( fgets(buf, 1023, pp) != NULL )
	{
	}
	pclose(pp);
	
	buf[ strlen(buf) - 1 ] = '\0';
	if ( strlen(buf) > 0 )
	{
		return buf;
	}
	else
	{
		free(buf);
		return NULL;
	}
}

int open_trackpoint(char *path)
{
	char sens[1024];
	int fd;
	sprintf(sens, "%s/sensitivity", path);
	return open(sens, O_RDWR);
}

void disable_trackpoint(int fd)
{
	if ( fd >= 0 )
		write(fd, "0\n", 2);
}

void enable_trackpoint(int fd)
{
	if ( fd >= 0 )
		write(fd, "128\n", 4);
}

int main(int argc, char **argv)
{
	/* preserve old uid... */
	int olduid = getuid();
	int oldgid = getgid();
	/* setuid */
	if ( setuid(0) != 0 )
	{
		perror("setuid");
		return 1;
	}
	if ( setgid(0) != 0 )
	{
		perror("setgid");
		return 1;
	}
	/* open the event device */
	int fd = open("/dev/input/by-path/platform-thinkpad_acpi-event", O_RDONLY);
	if ( fd < 0 )
	{
		perror("open");
		return 1;
	}
	/* Find the TrackPoint */
	char *tp = find_trackpoint();
	int tpfd = -1;
	if ( tp != NULL )
		tpfd = open_trackpoint(tp);
	
	/* and now we have our handle, so regress to the old user */
	if ( setgid(oldgid) != 0 )
	{
		perror("setgid (back to old)");
		return 1;
	}
	if ( setuid(olduid) != 0 )
	{
		perror("setuid (back to old)");
		return 1;
	}
	umask(0077);
	/* read loop */
	unsigned short result = TP_EVENT_NONE;
	while ( result != TP_EVENT_EOF )
	{
		result = scan_event(fd);
		if ( result == TP_EVENT_TABDOWN )
		{
			tablet_down();
			disable_trackpoint(tpfd);
		}
		else if ( result == TP_EVENT_TABUP )
		{
			tablet_up();
			enable_trackpoint(tpfd);
		}
	}
	close(fd);
	/* free the trackpoint */
	if ( tp != NULL )
	{
		close(tpfd);
		free(tp);
	}
	
	return 0;
}
