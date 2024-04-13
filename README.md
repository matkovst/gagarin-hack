# Гагарин хакатон

## Установка решения через Docker

Из папки проекта выполнить

```bash
docker build --tag gagarin_hack_r7 -f docker/Dockerfile .

# Прогон RTSP
docker run gagarin_hack_r7 <rtsp> <N> <K>

# Прогон FILE
docker run --volume=/path/to/video:/data gagarin_hack_r7 </data/source_file> <N> <K>
```

- rtsp/source_file - адрес живого потока/видеофайл
- N - окно для сбора плавающих статистик. Измеряется в количестве ключевых кадров. Эмпирические значения - от 3 до 12.
- K - допустимые колебания размера ключевого пакета вокруг среднего. Измеряется в стандартных отклонениях размера. Эмпирические значения - от 1 до 3.
