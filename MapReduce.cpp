#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <semaphore.h>

using namespace std;

//Initializing the data in a class to prevent the usage of global variables
class MapReduce {
private:
    string* DataChunks;             
    int TotalChunks;               
    string* Keys;                    
    int* Counts;                     
    int KeyCount;                  
    int MaxKeys;                   
    sem_t OutputSem;                 
    int MapPipe[2];                  

public:
    MapReduce(const string& InputData, int NumThreads) {
        sem_init(&OutputSem, 0, 1); 
        SplitData(InputData, NumThreads);
        if (pipe(MapPipe) == -1) {
            perror("Pipe creation failed");
            exit(1);
        }
        MaxKeys = 400; 
        KeyCount = 0;
        Keys = new string[MaxKeys];
        Counts = new int[MaxKeys];
        memset(Counts, 0, MaxKeys * sizeof(int));
    }

    ~MapReduce() {
        close(MapPipe[0]);
        close(MapPipe[1]);
        sem_destroy(&OutputSem); 
        delete[] DataChunks;
        delete[] Keys;
        delete[] Counts;
    }
    
//Function for splitting data into smaller chunks of data
void SplitData(const string& InputData, int NumThreads) {
    int TotalWords = CountWords(InputData);
    int ChunkSize = max(1, TotalWords / NumThreads);
    TotalChunks = NumThreads; // Divide into exactly NumThreads chunks
    DataChunks = new string[TotalChunks];

    int WordCount = 0, ChunkIndex = 0;
    string CurrentChunk, CurrentWord;

    for (size_t i = 0; i < InputData.size(); ++i) {
        char Ch = InputData[i];

        if (isspace(Ch)) {
            if (!CurrentWord.empty()) {
                CurrentChunk += CurrentWord + " ";
                CurrentWord.clear();
                WordCount++;

                // Move to the next chunk only if it's not the last one
                if (WordCount == ChunkSize && ChunkIndex < TotalChunks - 1) {
                    DataChunks[ChunkIndex++] = CurrentChunk;
                    CurrentChunk.clear();
                    WordCount = 0;
                }
            }
        } else {
            CurrentWord += Ch;
        }
    }

    //Adding the last word and chunk (if any)
    if (!CurrentWord.empty()) {
        CurrentChunk += CurrentWord + " ";
    }
    if (!CurrentChunk.empty()) {
        DataChunks[ChunkIndex++] = CurrentChunk;
    }

    //Filling empty chunks for alignment
    while (ChunkIndex < TotalChunks) {
        DataChunks[ChunkIndex++] = "";
    }
}



    //Function for counting the words in the text
    int CountWords(const string& Str) const {
        int Count = 0;
        bool InWord = false;
        for (char Ch : Str) {
            if (isspace(Ch)) {
                if (InWord) {
                    Count++;
                    InWord = false;
                }
            } else {
                InWord = true;
            }
        }
        if (InWord) Count++;
        return Count;
    }

    //Function for updating the values of the keys
    void AddOrUpdateKey(const string& Key, int Value) {
        sem_wait(&OutputSem); // Lock semaphore

        for (int i = 0; i < KeyCount; ++i) {
            if (Keys[i] == Key) {
                Counts[i] += Value;
                sem_post(&OutputSem); // Unlock semaphore
                return;
            }
        }

        if (KeyCount == MaxKeys) {
            GrowKeyArray();
        }
        Keys[KeyCount] = Key;
        Counts[KeyCount] = Value;
        KeyCount++;

        sem_post(&OutputSem); // Unlock semaphore
    }

    //Function to keep count of the keys
    void GrowKeyArray() {
        MaxKeys *= 2;
        string* NewKeys = new string[MaxKeys];
        int* NewCounts = new int[MaxKeys];
        for (int i = 0; i < KeyCount; ++i) {
            NewKeys[i] = Keys[i];
            NewCounts[i] = Counts[i];
        }
        delete[] Keys;
        delete[] Counts;
        Keys = NewKeys;
        Counts = NewCounts;
    }

    //Mapping function
    static void* MapPhase(void* Args) {
        auto* Params = static_cast<pair<string, int>*>(Args);
        string Chunk = Params->first;
        int WriteFd = Params->second;

        string Word;
        string Buffer;
        for (char Ch : Chunk) {
            if (isspace(Ch)) {
                if (!Word.empty()) {
                    Buffer += Word + " 1\n";
                    Word.clear();
                }
            } else {
                Word += Ch;
            }
        }
        if (!Word.empty()) {
            Buffer += Word + " 1\n";
        }

        write(WriteFd, Buffer.c_str(), Buffer.length());
        return nullptr;
    }

    //Function for implementing shuffling
    void ShufflePhase() {
        char Buffer[256];
        while (true) {
            int BytesRead = read(MapPipe[0], Buffer, sizeof(Buffer) - 1);
            if (BytesRead <= 0) break;

            Buffer[BytesRead] = '\0';
            char Key[50];
            int Value;
            const char* Token = strtok(Buffer, "\n");
            while (Token) {
                sscanf(Token, "%s %d", Key, &Value);
                AddOrUpdateKey(Key, Value);
                Token = strtok(nullptr, "\n");
            }
        }
    }

    //Processes grouped data for final results
    void ReducePhase() {
        for (int i = 0; i < KeyCount; ++i) {
        if(Keys[i]!="1")
            cout << "(" << Keys[i] << ", " << Counts[i] << ")\n";
        }
    }

    //Function for implementing the initation of mapreduce process
    void Execute() {
        pthread_t* MapperThreads = new pthread_t[TotalChunks];
        pair<string, int>* MapperArgs = new pair<string, int>[TotalChunks];

        for (int i = 0; i < TotalChunks; ++i) {
            MapperArgs[i] = {DataChunks[i], MapPipe[1]};
            pthread_create(&MapperThreads[i], nullptr, &MapReduce::MapPhase, &MapperArgs[i]);
        }

        for (int i = 0; i < TotalChunks; ++i) {
            pthread_join(MapperThreads[i], nullptr);
        }
        close(MapPipe[1]); 

        ShufflePhase();

        ReducePhase();
    }
};

int main() {
    string TestCase = "pizza burger apple bread milk cheese salad soup pasta olive tomato bread pizza cheese olive burger soup salad pasta pizza bread milk apple pizza burger salad cheese soup olive burger pasta milk tomato bread soup cheese tomato pasta pizza apple salad olive spinach";
    int NumThreads = 2;

    MapReduce MapReduceInstance(TestCase, NumThreads);
    MapReduceInstance.Execute();
    return 0;
}

