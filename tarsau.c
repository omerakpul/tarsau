#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void handleFileError(const char *fileName, FILE *output) {
    printf("error: Can't open file: %s\n", fileName);
    fclose(output);
    exit(EXIT_FAILURE);
}

void checkAsciiFormat(const char *fileName, FILE *input) {
    int ch_size;
    while ((ch_size = fgetc(input)) != EOF) {
        if (ch_size > 127) {
            printf("%s error: input character must be ASCII format \n", fileName);
            fclose(input);
            exit(EXIT_FAILURE);
        }
    }
}

void mergeFiles(FILE *output, const char *format, const char *fileName, int permissions, int size) {
    char record[256];
    sprintf(record, format, fileName, permissions, size);
    fwrite(record, 1, strlen(record), output);
}

void tarsau_b(int argc, char *argv[], char *outputf, const char *format) {
    FILE *output = fopen(outputf, "w");

    if (output == NULL) {
        handleFileError(outputf, output);
    }

    int total_size = 0;

    for (int i=2; i < argc; i++){

        if (strcmp(argv[i], "-o") == 0) {
            i++;
        }

        FILE *inputfs = fopen(argv[i], "r");
        fseek(inputfs, 0, SEEK_END);
        int file_size = ftell(inputfs);
        fseek(inputfs, 0, SEEK_SET);

        mergeFiles(output, "|%s,%o,%d|" ,argv[i], 0644, file_size );

        if(i==argc-1){
            fprintf(output,"\n");
        }
        fclose(inputfs);

    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            i++;
        }

        FILE *inputfs = fopen(argv[i], "r");

        if (inputfs == NULL) {
            handleFileError(argv[i], output);
        }

        checkAsciiFormat(argv[i], inputfs);

        fseek(inputfs, 0, SEEK_END);
        int file_size = ftell(inputfs);
        fseek(inputfs, 0, SEEK_SET);

        if (total_size + file_size > 200) {
            printf("total size of files exceeds 200MB \n");
            fclose(inputfs);
            fclose(output);
            exit(EXIT_FAILURE);
        }

        

        char temp[file_size];
        fread(temp, 1, file_size, inputfs);
        fwrite(temp, 1, file_size, output);

        total_size += file_size;

    

        fclose(inputfs);
    }

    fclose(output);
    printf("The files have been merged.\n");
}

void handleFileErrorExtract(const char *fileName) {
    printf("error: Unable to open file: %s\n", fileName);
    exit(EXIT_FAILURE);
}

void createDirectory(const char *output_directory) {
    mkdir(output_directory, 0777);
}

void extractAndWriteFile(FILE *archive_fp, int toplam, const char *output_directory, const char *file_name, int size) {
    char output_path[256];
    snprintf(output_path, sizeof(output_path), "%s/%s", output_directory, file_name);

    FILE *out_fp = fopen(output_path, "w");
    if (out_fp == NULL) {
        handleFileErrorExtract(file_name);
    }

    fseek(archive_fp, toplam, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fputc(fgetc(archive_fp), out_fp);
    }

    fclose(out_fp);
}

void extractFileContent(FILE *archive_fp, int toplam, const char *output_directory) {
    char file_list[200];
    fread(file_list, 1, sizeof(file_list), archive_fp);

    char *file_data = strchr(file_list, '|');
    while (file_data != NULL) {
        file_data++;

        char *file_info_end = strchr(file_data, '|');
        if (file_info_end != NULL) {
            *file_info_end = '\0';

            char *file_name = strtok(file_data, ",");
            char *file_perm = strtok(NULL, ",");
            char *file_content = strtok(NULL, "|");

            if (file_name != NULL && file_content != NULL) {
                extractAndWriteFile(archive_fp, toplam, output_directory, file_name, atoi(file_content));
                toplam += atoi(file_content);
            } else {
                printf("wrong file format: %s\n", file_data);
            }

            *file_info_end = '|';
            file_data = strchr(file_info_end + 1, '|');
        } else {
            printf("wrong file format: '|' separator are not found.\n");
            break;
        }
    }
}

void tarsau_a(char *archive_file, char *output_directory) {
    FILE *in_fp = fopen(archive_file, "r");
    if (in_fp == NULL) {
        printf("archive could be corrupt\n");
        return;
    }
    fclose(in_fp);

    createDirectory(output_directory);

    FILE *archive_fp = fopen(archive_file, "r");
    if (archive_fp == NULL) {
        printf("error: opening archive file.\n");
        return;
    }

    int toplam = 0;
    extractFileContent(archive_fp, toplam, output_directory);

    fclose(archive_fp);

    printf("Files opened in the directory: %s\n", output_directory);
}

int main(int argc, char *argv[]) {

    if ((argc < 3 && (strcmp(argv[1], "-b") || strcmp(argv[1], "-a"))) != 0) {
        printf("tarsau -b is for merging the files and tarsau -a is for separating files\n");
        printf("you should select files for these operations\n");

        return 1;
    }

    char *outputf = "a.sau";
    char *archive_file = argv[2];
    char *output_directory = argv[3];

    for (int i = 2; i < argc; i++) {

        if (strcmp(argv[i], "-o") == 0) {

            if (argv[i + 1] != NULL) {
                outputf = argv[i + 1];
                i++;
            } else {
                printf("error: missing filename after ‘-o’\n");
                return 1;
            }
        }
    }

    if (strcmp(argv[1], "-b") == 0) {
        if (argc >= 32) {
            printf("max number of input files is 32\n");
            return 1;
        }
     

        tarsau_b(argc, argv, outputf, "|%s,%o,%d|");
    } else {
        tarsau_a(archive_file, output_directory);
    }

    return 1;
}