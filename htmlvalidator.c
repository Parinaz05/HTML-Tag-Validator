#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h> // For INT_MAX

#define MAX_LEN 50

//valid tags and their priority 
const char *validTags[] = {"html", "head", "title", "body", "h1", "h2", "h3", "p", "br", "hr", "img", "ul", "ol", "li", "table", "tr", "th", "td", "form"};
const int tagsPriority[] = {1, 2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 5, 8, 8, 8, 5};
const int numTags = sizeof(validTags) / sizeof(validTags[0]);

//Empty tags 
const char *emptyTags[] = {"br", "img", "hr"};
const int numEmptyTags = sizeof(emptyTags) / sizeof(emptyTags[0]);

//Node Structure 
typedef struct TagNode {
    char tag[MAX_LEN];
    struct TagNode *prev, *next;
} TagNode;

// extract tag name 
void extractTagName(const char *input, char *output) {
    int i = 0, j = 0;
    if (input [0] == '<') i++;
    
    int isClosing = 0;
    if(input[i] == '/'){
        isClosing = 1;
        i++;
    }

    while (input[i] != '>' && input[i] != '\0' && input[i] != ' '){
        output[j++] = tolower(input[i]);
        i++;
    }

    if(input[i] != '>') {
        output[0] = '\0';
        return;
    }
    
    output[j] = '\0';

    if(isClosing){
        memmove(output + 1, output, j + 1);
        output[0] = '/';
    }
 }

//check if a tag is valid 
int isValidTag(const char *tag) {
    int startIndex = (tag[0] == '/') ? 1 : 0;
    for (int i = 0; i < numTags; i++){
        if(strcmp(tag + startIndex, validTags[i]) == 0){
            return 1;
        }
    }
    return 0;
}

//get tag priority
int getTagPriority(const char *tag) {
    for (int i = 0; i < numTags; i++){
        if(strcmp(tag, validTags[i]) == 0){
            return tagsPriority[i];
        } 
    }
    return -1;
}

int isEmptyTag(const char *tag){
    for (int i = 0; i < numEmptyTags; i++){
        if(strcmp(tag, emptyTags[i]) == 0){
            return 1;
        }
    }
    return 0;
}

//cheack if two tags match
int isMatching(const char *openTag, const char *closeTag) {
    return (closeTag[0] == '/' && strcmp(openTag, closeTag + 1) == 0);
}

//inserts tag at the end of the list 
TagNode* insertTag(TagNode **head, const char *tag) {
    char cleanTag[MAX_LEN];
    extractTagName(tag, cleanTag);

    if(strlen(cleanTag) == 0){
        printf("Error: Invalid tag format <%s>.\n", tag);
        return NULL;
    }

    if (!isValidTag(cleanTag)) {
        printf("Error: Invalid tag <%s>\n", cleanTag);
        return NULL;
    }

    TagNode *newNode = (TagNode *)malloc(sizeof(TagNode));
    if (!newNode) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    strcpy(newNode->tag, cleanTag);
    newNode->next = NULL;

    if (*head == NULL) {
        newNode->prev = NULL;
        *head = newNode;
    } else {
        TagNode *temp = *head;
        while (temp->next) {
            if (cleanTag[0] != '/' && temp->tag[0] != '/' && getTagPriority(cleanTag) < getTagPriority(temp->tag)) {
                printf("Semantic error: <%s> should not appear after <%s>\n", cleanTag, temp->tag);
                free(newNode);
                return NULL;
            } 
            temp = temp->next;
        }
        temp->next = newNode;
        newNode->prev = temp;
    }
    return newNode;
}

//deletes both opening and closing tags 
void deleteTag(TagNode **head, const char *tag) {
    if(*head == NULL) {
        printf("Error: No tags to delete\n");
        return;
    }

    TagNode *temp = *head;
    
    char closingTag[MAX_LEN] = "/";

    strcat(closingTag, tag);

    while (temp){
      if (strcmp(temp->tag, tag) == 0 || strcmp(temp->tag, closingTag) == 0){
          TagNode *nextNode = temp -> next;
          
          if(temp->prev) temp->prev->next = temp->next;
          if(temp->next) temp->next->prev = temp->prev;
          
          if(temp == *head) *head = temp->next;
          
          free(temp);
          temp = nextNode;
      } else {
          temp = temp -> next;
      }
      
    }
}

void validate(TagNode **head) {
    TagNode *temp = *head;
    TagNode *openTags[100];
    int openTagCount = 0;
    int errorFlag = 0;
    int expectedPriority = 1; //start with html priority
    int headEncountered = 0; //Track if head tag was found
    int titleEncountered = 0; //Track if title tag was found

    if (*head && strcmp((*head)->tag, "html") != 0) {
        printf("Semantic error: Missing <html> tag.\n");
        errorFlag = 1;
    }
    
    while (temp) {
        if (strlen(temp->tag) == 0){
            printf("Error: Invalid or incomplete tag found. <%s>\n", temp->tag);
            errorFlag = 1;
        }

        int currentPriority = getTagPriority(temp->tag[0] == '/' ? temp->tag + 1 : temp->tag);
        
        if (temp->tag[0] != '/') {
            if (currentPriority == 2) {
                headEncountered = 1;
            } else if (currentPriority == 3) {
                titleEncountered = 1;
            }
            
            if(currentPriority != expectedPriority){
                if(headEncountered == 1 && expectedPriority == 3 && currentPriority == 4){
                    expectedPriority = 4; //body follows head if title is not present
                } else if(headEncountered == 0 && titleEncountered == 0) {
                    //skip head and title priority checks
                    if (currentPriority > 0) {//check if current tag has a valid priority
                        expectedPriority = currentPriority + 1;
                    } else {
                        expectedPriority = 1;
                    }
                } else if (currentPriority != 2 || headEncountered == 1) {
                    printf("Semantic error: <%s> should appear after <%s> or be in correct order.\n", temp->tag, validTags[expectedPriority -1]);
                errorFlag = 1;
                }
            }
            openTags[openTagCount] = temp;
            openTagCount++;
            expectedPriority++; 
        } else { // closing tag
            int matchFound = 0;
            for (int i = openTagCount - 1; i >= 0; i--) {
                if(isMatching(openTags[i]->tag, temp->tag)){
                    openTagCount = i;
                    matchFound = 1;
                    expectedPriority = getTagPriority(openTags[i]->tag) + 1;
                    break;
                }
            }
            if (!matchFound) {
                printf("Error: Unmatched closing tag </%s>\n", temp->tag);
                errorFlag = 1;
            }
        }
        temp = temp->next;
    }

    for (int i = 0; i < openTagCount; i++) {
        printf("Error: Unmatched tag </%s>\n", openTags[i]->tag);
        errorFlag = 1;
    }

    if (!errorFlag) {
        printf("Code is valid.\n");
    }
}


//print tags in sequence
void printTags(TagNode *head){
    TagNode *temp = head;
    printf("Tags in sequence: ");
    while (temp){
        printf("<%s> ", temp->tag);
        temp = temp -> next;
    }
    printf("\n");
}

void freeTagList(TagNode *head) {
    while (head) {
        TagNode *temp = head;
        head = head->next;
        free(temp);
    }
}

int main() {
    TagNode *head = NULL;
    char input[MAX_LEN];
    int choice;

    while (1) {
        printf("\nHTML Tag Validator\n");
        printf("1. Insert Tag\n2. Delete Tag\n3. Validate Code\n4. Display Tags\n5. Exit\nEnter choice: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1:
                printf("Enter HTML tag (e.g., <div>, <p>, <a>): ");
                if (fgets(input, MAX_LEN, stdin)) {
                    input[strcspn(input, "\n")] = 0; //remove new line

                    char tag[MAX_LEN];
                    char *start = input;
                    while (*start) {
                        if (*start == '<') {
                            char *end = strchr(start, '>');
                            if (end) {
                                strncpy(tag, start, end - start + 1);
                                tag[end - start + 1] = '\0';
                                if (insertTag(&head, tag) == NULL) {
                                    printf("Error: Failed to insert tag %s\n", tag);
                                }
                                start = end + 1;
                            } else{
                                printf("Error: Incomplete tag found: %s\n",start);
                                break;
                            }
                        } else {
                                start++;
                        }
                    }
                }
                break;
            case 2:
                printf("Enter tag to delete (e.g., <div>, <p>, <a>): ");
                if(fgets(input, MAX_LEN, stdin)){
                    input[strcspn(input, "\n")] = 0; //remove new line
                    char tag[MAX_LEN];
                    extractTagName(input, tag);
                    deleteTag(&head, tag);
                }
                printf("Warning! Both pairs of this tag will be deleted\n");
                break;

            case 3:
                validate(&head);
                break;

            case 4:
                printTags(head);
                break;

            case 5: 
                freeTagList(head);
                exit(0);

            default:
                printf("Invalid choice. Please try again.\n");
            break;
        }
    }
    return 0;
}
