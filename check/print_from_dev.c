#include <unistd.h>
#include <fcntl.h>

#include <linux/input.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MOUSE_LEFT_BTN		(1 << 0)
#define MOUSE_RIGHT_BTN		(1 << 1)
#define MOUSE_MIDDLE_BTN	(1 << 2)

typedef struct		mouse_event
{
  struct timeval	time;
  unsigned short	type;
  unsigned short	code;
  unsigned int		value;
} __attribute__((packed)) mouse_event;

void			print_mouse(const char* device)
{
  mouse_event		event;
  unsigned long		nb_event	= 0;
  unsigned int		nb_read		= 0;
  int			mouse_file	= -1;

  if ((mouse_file = open(device, O_RDONLY)) == -1)
    {
      printf("Cannot open device.\n");
      return;
    }

  memset(&event, 0x01, sizeof(mouse_event));

  while (1)
    {
      nb_read = read(mouse_file, &event, sizeof(mouse_event));

      printf("Event received (%lu):\n", nb_event);
      if (event.type == 3)
	{
	  printf("\t event: ABS.\n");
	  if (event.code == ABS_X)
	    {
	      printf("\t X: %d.\n", event.value);
	    }
	  if (event.code == ABS_Y)
	    {
	      printf("\t Y: %d.\n", event.value);
	    }
	}
      ++nb_event;
    }
}

int			main(int argc, char** argv)
{
  if (argc == 2)
    print_mouse(argv[1]);
  else
    return (1);

  return (0);
}
