library("RJDBC")
library("DBI")
library("rJava")
DIR <- "/home/tina/jaguar/lib/"
#install.packages("arules", lib="//home/tina/jaguar/lib/rlib")
#library(arules,lib.loc="/home/tina/jaguar/lib/rlib",warn.conflicts = FALSE)
#install.packages("e1071",lib="//home/tina/jaguar/lib/rlib")
install.packages("rpart",lib="//home/tina/jaguar/lib/rlib")
drv <- try(JDBC("com.jaguar.jdbc.JaguarDriver", paste0(DIR, 'jaguar-jdbc-2.0.jar')))
#.jaddClassPath(paste(DIR, "jaguar-jdbc-2.0.jar"))
drv <- JDBC(driverClass = "com.jaguar.jdbc.JaguarDriver")
con <- dbConnect(drv, "jdbc:jaguar://192.168.7.120:8875/test", "admin", "jaguarjaguarjaguar")


#Retrive from Jaguar
table <- dbGetQuery(con, "select * from cancer;")
table <- table[,-ncol(table)]
table <- na.omit(table)
table$class <- ifelse(table$class == "2", "benign",
                          ifelse(table$class == "4", "malignant", NA))
summary(table)

#Missing data
table[table == "?"] <-NA
length(which(is.na(table)))

#Pick training and testing set
## 75% of the sample size
smp_size <- floor(0.75 * nrow(table))

## set the seed to make your partition reproducible
set.seed(123)
train_ind <- sample(seq_len(nrow(table)), size = smp_size)
train <- table[train_ind, ]
test <- table[-train_ind, ]
nrow(train)
nrow(test)

# Decision Tree
library(rpart,lib.loc="/home/tina/jaguar/lib/rlib",warn.conflicts = FALSE)
library(rpart.plot,lib.loc="/home/tina/jaguar/lib/rlib",warn.conflicts = FALSE)


# Naive Bayes
library(e1071,lib.loc="/home/tina/jaguar/lib/rlib",warn.conflicts = FALSE)

#naiveBayes(x, y,laplace=0)
n<-ncol(table)
x <-train[,1:n-1]
y <-train[,n]
#model<-naiveBayes(x,y)
#model
#table(predict(model,train[,-n]),train[,n])
#predict(model,test[1,])

model <-naiveBayes(class~. , data = train)
model
#pred <- predict(model, test)





dbDisconnect(con)



