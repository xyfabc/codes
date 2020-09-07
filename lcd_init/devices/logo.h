#ifndef LOGO_H
#define LOGO_H


class logo
{
public:
    logo();
};

int ShowBmp(char *path, int fbfd, struct fb_var_screeninfo *vinfo, char *fbp);

#endif // LOGO_H
