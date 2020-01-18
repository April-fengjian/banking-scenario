//
//Zhiqi Bei(#250916374)
//zbei@uwo.ca

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

struct account{
    char name[1024];
    char type[1024];
    int depositFund;
    int withdrawFund;
    int transFund;
    int transLimit;
    int transNum;
    int additionalFund;
    int overdraft;
    int balance;
};

struct action{
    char myaccount[100];         //store the name of the account
    int money;
    int total;                  //the total number of account in the account list
    char trans_account[100];
    char type;                  //the type of the action: withdraw, deposit or transfer
};

//the list is to store all the account
struct account list[1000];
int deposit_num;
pthread_mutex_t lock;

//thread functon for deposit money
void *deposit_money(void *action){
    struct action *dep = (struct action *)action;
    struct action deposit = *dep;
    struct account *myaccount = NULL;
    //find the associate account depend on the account name
    for(int j=0; j<deposit.total;j++){
        if(strcmp(list[j].name, deposit.myaccount)==0){
            myaccount = &list[j];
        }
    }
    pthread_mutex_lock(&lock);
    int myaddition = 0;
    
    //if the translimit is exceed, calculate the addition fee(myaddition)
    myaccount->transLimit--;
    if(myaccount->transLimit < 0){
        myaddition = myaccount->additionalFund;
    }
    //calculate the new balance after deposit
    int newbalance = myaccount->balance + deposit.money - myaddition - myaccount->depositFund;
    myaccount->balance = newbalance;
    pthread_mutex_unlock(&lock);
    return 0;
}

//thread function for withdraw money
void *withdraw_money(void *action){
    struct action *dep = (struct action *)action;
    struct action withdraw = *dep;
    struct account *myaccount = NULL;
    //find the associate account depend on the account name
    for(int j=0; j<withdraw.total;j++){
        if(strcmp(list[j].name, withdraw.myaccount)==0){
            myaccount = &list[j];
        }
    }
    pthread_mutex_lock(&lock);
    
    //calculate the additional fee if the translimit is exceed
    int myaddition = 0;
    myaccount->transLimit--;
    if(myaccount->transLimit < 0){
        myaddition = myaccount->additionalFund;
    }
    
    //if account balance > withdraw fund + withdraw money
    //means this withdraw will not cost a overdraft fee
    if(withdraw.money <= (myaccount->balance - myaccount->withdrawFund)){
        int newbalance = myaccount->balance - withdraw.money - myaccount->withdrawFund - myaddition;
        myaccount->balance = newbalance;
    }else{
        if(myaccount->overdraft != 0){
            //calculate the fee after withdraw
            int fee = ((myaccount->balance - withdraw.money-myaccount->withdrawFund - myaddition)/(-500) + 1) * myaccount->overdraft;
            int paid = 0;
            //if the account balance is <0
            //means the account had paid some fee for overdraft, so calculate the extra overdrat fee
            if(myaccount->balance < 0){
                //calculate the fee that already paid,
                paid = (myaccount->balance/(-500)+1)*myaccount->overdraft;
                //means the account must paid extera fee
                if(fee >= paid){
                    int temp = fee;
                    fee = fee - paid;
                    paid = temp;
                }
            }else{
                paid = fee;
            }
            int newbalance = myaccount->balance - withdraw.money - myaccount->withdrawFund - fee - myaddition;
            //check if the balance is overdraft into next stage
            //and calculate the new fee is needed
            int newFee = (newbalance/(-500)+1) * myaccount->overdraft;
            //if nwefee > paid means the account need pay an extera fee again
            if(newFee > paid){
                newbalance = newbalance - (newFee - paid);
            }
            //after all the calculation, if the balance will <-5000, this request will denied
            if(newbalance > -5000){
                myaccount->balance = newbalance;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

//the thread function for transfer money
void *transfer_money(void *action){
    struct action *dep = (struct action *)action;
    struct action transfer = *dep;
    struct account *myaccount = NULL;
    struct account *trans_account = NULL;
    //find the associate account depend on the account name
    for(int j=0; j<transfer.total;j++){
        if(strcmp(list[j].name, transfer.myaccount)==0){
            myaccount = &list[j];
        }
        else if(strcmp(list[j].name, transfer.trans_account)==0){
            trans_account = &list[j];
        }
    }
    pthread_mutex_lock(&lock);
    
    //calculate the additional fee for both account if the translimit is exceed
    myaccount->transLimit --;
    trans_account->transLimit--;
    int myaddition = 0;
    int trans_addition = 0;
    if(myaccount->transLimit < 0){
        myaddition = myaccount->additionalFund;;
    }
    if(trans_account->transLimit < 0){
        trans_addition = trans_account->additionalFund;
    }
    //if account balance > transfer fund + transfer money
    //means this withdraw will not cost a overdraft fee
    if((myaccount->balance - myaccount->transFund) > transfer.money){
        int newbalance = myaccount->balance - transfer.money - myaccount->transFund - myaddition;
        myaccount->balance = newbalance;
        newbalance = trans_account->balance + transfer.money - trans_account->transFund - trans_addition;
        trans_account->balance = newbalance;

    }else{
        if(myaccount->overdraft != 0){
            //calculate the fee after transfer
            int fee = ((myaccount->balance - transfer.money - myaccount->overdraft - myaddition)/(-500) + 1) * myaccount->overdraft;
            int paid = 0;
            //if current account balance < 0
            //means the account had already paid some fee for overdraft
            if(myaccount->balance < 0){
                //calculate the fee that already paid
                paid = (myaccount->balance/(-500)+1)*myaccount->overdraft;
                //means the account must paid extera fee
                if(fee >= paid){
                    int temp = fee;
                    fee = fee - paid;
                    paid = temp;
                }
            }else{
                paid = fee;
            }
            //calculate the new balance after this transfer
            int newbalance = myaccount->balance - transfer.money - myaccount->transFund - myaddition - fee;
            //check if the balance is overdraft into next stage
            //and calculate the new fee is needed
            int newFee = (newbalance/(-500)+1) * myaccount->overdraft;
            //if nwefee > paid means the account need pay an extera fee again
            if(newFee > paid){
                newbalance = newbalance - (newFee - paid);
            }
            //after all the calculation, if the balance will <-5000, this request will denied
            if(newbalance > -5000){
                myaccount->balance = newbalance;
                newbalance = trans_account->balance + transfer.money - trans_account->transFund - trans_addition;
                trans_account->balance = newbalance;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}
//funtion for calculate the last line number of the depositors' action
int last_dep_position(FILE *inputfile){
    char line[1024];
    int position = 1;
    int d_position = 0;
    fgets(line, 1024, inputfile);
    while(!feof(inputfile)){
        if(line[0] == 'd'){
            d_position = position;
        }
        fgets(line, 1024, inputfile);
        position++;
    }
    return d_position;
}

pthread_t threads[1000];
int thread_num = 0;

int main(int argc, const char * argv[]) {
    //open file, after calculate the last line number of the depositor, close the inputfile, and open it again
    FILE *inputfile, *outputfile = NULL;
    inputfile = fopen("assignment_3_input_file.txt", "r");
    int d_position = last_dep_position(inputfile);
    fclose(inputfile);
    inputfile = fopen("assignment_3_input_file.txt", "r");
    outputfile = fopen("assignment_3_output_file.txt", "w+");
    
    char line[1024];
    char * c;
    
    int num = 0;
    if(inputfile == NULL){
        printf("There is no such file\n");
        return 0;
    }
    
    fgets(line, 1024, inputfile);
    //the position of the line which is now reading
    int position = 1;

    if (pthread_mutex_init(&lock, NULL) != 0){
        printf("\n mutex init failed\n");
        return 1;
    }
    
    struct action client[1000];
    int client_num = 0;
    while(!feof(inputfile)){
        
        c = strtok(line," ");
        
        struct account account;
        char info[1000][1000];
        int index = 0;
        //store the infomation into a array
        while(c != NULL){
            strcpy(info[index], c);
            index = index + 1;
            c = strtok(NULL," ");
        }
        //if is 'a' create account
        if(info[0][0] == 'a'){
            //create account
            strcpy(account.name, info[0]);
            strcpy(account.type, info[2]);
            account.depositFund = atoi(info[4]);
            account.withdrawFund = atoi(info[6]);
            account.transFund = atoi(info[8]);
            account.transLimit = atoi(info[10]);
            account.transLimit = atoi(info[10]);
            account.additionalFund = atoi(info[11]);
            if(*info[13] == 'Y'){
                account.overdraft = atoi(info[14]);
            }else{
                account.overdraft = 0;
            }
            // put the account into the account list
            list[num] = account;
            num = num + 1;
            
        }
        //if is depositor
        if(info[0][0] == 'd'){
            int err_thread;
            for(int i=1;i<index;i=i+3){
                if(*info[i] == 'd'){
                    //store the infomation that need to do deposit
                    //(name of myaccount, the number of money need to deposit and total number of account in the list)
                    struct action deposit;
                    deposit.money = atoi(info[i+2]);
                    strcpy(deposit.myaccount, info[i+1]);
                    deposit.total = num;
                    
                    err_thread = pthread_create(&threads[thread_num], NULL, &deposit_money, &deposit);
                    thread_num ++;

                    if (err_thread != 0){
                        printf("\n Error creating thread %d", i);
                    }
                    for (int i = 0; i< thread_num; i++)
                        pthread_join(threads[i], NULL);
                }
            }
        }
        //if is client, and the client must do their action after all the depositor had finish
        if (info[0][0] == 'c'&position > d_position){
            for(int i=1;i<index;i=i+3){
                //if the client want deposit money
                if(*info[i] == 'd'){
                    //store the infomation that need to do deposit
                    //name of myaccount, the number of money need to deposit, the total number of account in the list and the type of action need to do with the account (deposit)
                    struct action deposit;
                    deposit.money = atoi(info[i+2]);
                    strcpy(deposit.myaccount, info[i+1]);
                    deposit.total = num;
                    deposit.type = 'd';
                    //save the action into a list for later create thread
                    client[client_num] = deposit;
                    client_num++;
                    
                }
                //if the client want withdraw money
                if(*info[i] == 'w'){
                    //store the infomation that need to do withdraw
                    //name of myaccount, the number of money need to withdraw, the total number of account in the list and the type of action need to do with the account (withdraw)
                    struct action withdraw;
                    withdraw.money = atoi(info[i+2]);
                    strcpy(withdraw.myaccount, info[i+1]);
                    withdraw.total = num;
                    withdraw.type = 'w';
                    //save the action into a list for later create thread
                    client[client_num] = withdraw;
                    client_num++;
                    
                }
                //if the client want transfer money
                if(*info[i] == 't'){
                    //store the infomation that need to do tranfer
                    //name of myaccount and the account need to transfer with, the number of money need to transfer, the total number of account in the list and the type of action need to do with the account (tranfer)
                    struct action transfer;
                    transfer.money = atoi(info[i+3]);
                    strcpy(transfer.myaccount, info[i+1]);
                    strcpy(transfer.trans_account, info[i+2]);
                    transfer.total = num;
                    transfer.type = 't';
                    i = i + 1;
                    //save the action into a list for later create thread
                    client[client_num] = transfer;
                    client_num++;
                }
            }
        }
        position++;
        fgets(line, 1024, inputfile);
        
    }
    //create thread base on the action in the list
    for(int i=0;i<client_num;i++){
        if(client[i].type == 'd'){
            pthread_create(&threads[thread_num], NULL, &deposit_money, &client[i]);
        }
        if (client[i].type == 'w') {
            pthread_create(&threads[thread_num], NULL, &withdraw_money, &client[i]);
        }
        if (client[i].type == 't') {
            pthread_create(&threads[thread_num], NULL, &transfer_money, &client[i]);
        }
        thread_num++;
    }
    for (int i = 0; i< thread_num; i++){
        pthread_join(threads[i], NULL);
    }
    for(int k=0; k<num;k++){
        fprintf(outputfile, "%s type %s %d\n",list[k].name,list[k].type,list[k].balance);
    }
    pthread_mutex_destroy(&lock);
    //close the file at list
    fclose(inputfile);
    fclose(outputfile);
}
