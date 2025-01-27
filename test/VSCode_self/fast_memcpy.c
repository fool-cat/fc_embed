
void fast_memcpy(void* dst, void* src, uint32_t size)
{
    // 判断原地址和目标地址的最小对齐单位
    size_t min_align;
    if ((((size_t)dst & 0x7) == 0) && (((size_t)src & 0x7) == 0))
    {
        printf("this align 8\r\n");
        min_align = 8;
    }
    else if ((((size_t)dst & 0x3) == 0) && (((size_t)src & 0x3) == 0))
    {
        printf("this align 4\r\n");
        min_align = 4;
    }
    else if ((((size_t)dst & 0x1) == 0) && (((size_t)src & 0x1) == 0))
    {
        printf("this align 2\r\n");
        min_align = 2;
    }
    else
    {
        printf("this align 1\r\n");
        min_align = 1;
    }

    switch (min_align)
    {
    case 8:
    {
        // 节省变量开辟,min_align复用
        min_align = size >> 3;  // 等效于size/8
        while (min_align--)     // 减少变量开辟
        {
            *((uint64_t*)dst) = *((uint64_t*)src);
            dst = (uint64_t*)dst + 1;
            src = (uint64_t*)src + 1;
        }

        // 剩下的单字节拷贝
        min_align = size & 0x07;
        printf("single copy: %d\r\n", min_align);
        while (min_align--)  // 减少变量开辟
        {
            *((uint8_t*)dst) = *((uint8_t*)src);
            dst = (uint8_t*)dst + 1;
            src = (uint8_t*)src + 1;
        }
    }
    break;

    case 4:
        min_align = size >> 2;
        while (min_align--)
        {
            *((uint32_t*)dst) = *((uint32_t*)src);
            dst = (uint32_t*)dst + 1;
            src = (uint32_t*)src + 1;
        }
        min_align = size & 0x03;
        printf("single copy: %d\r\n", min_align);
        while (min_align--)
        {
            *((uint8_t*)dst) = *((uint8_t*)src);
            dst = (uint8_t*)dst + 1;
            src = (uint8_t*)src + 1;
        }
        break;

    case 2:
        min_align = size >> 1;
        while (min_align--)
        {
            *((uint16_t*)dst) = *((uint16_t*)src);
            dst = (uint16_t*)dst + 1;
            src = (uint16_t*)src + 1;
        }
        min_align = size & 0x01;
        printf("single copy: %d\r\n", min_align);
        while (min_align--)
        {
            *((uint8_t*)dst) = *((uint8_t*)src);
            dst = (uint8_t*)dst + 1;
            src = (uint8_t*)src + 1;
        }
        break;

    case 1:
        min_align = size;
        while (min_align--)
        {
            *((uint8_t*)dst) = *((uint8_t*)src);
            dst = (uint8_t*)dst + 1;
            src = (uint8_t*)src + 1;
        }
        break;
    }
}

void test()
{
    char src[15] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    char dst[15] = {0};

    fast_memcpy(dst, src, sizeof(src));

    for (int i = 0; i < sizeof(src); i++)
    {
        printf("%d ", dst[i]);
    }
    printf("\r\n");

    // 尝试非对齐
    memset(dst, 0, sizeof(dst));
    fast_memcpy(dst + 1, src + 1, sizeof(src) - sizeof(char));

    for (int i = 0; i < sizeof(src); i++)
    {
        printf("%d ", dst[i]);
    }
    printf("\r\n");
}
