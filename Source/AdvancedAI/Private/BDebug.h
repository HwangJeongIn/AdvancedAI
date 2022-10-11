#include "CoreMinimal.h"

//DECLARE_LOG_CATEGORY_EXTERN(ProjectB, Log, All);

#define B_LOG_CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))
// 가변인자 매크로 ... <=> __VA_ARGS__와 대응 // __VA_ARGS__앞에 ##를 붙이면 불필요한 쉼표가 자동으로 삭제 
#define B_LOG_BASE(Verbosity, Format, ...) UE_LOG(LogTemp, Verbosity, TEXT("%s => %s"), *B_LOG_CALLINFO, *FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define B_LOG_DEV(Format, ...) B_LOG_BASE(Warning, "[LOG] DESC : %s", *FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define B_ASSERT_DEV(Expr, Format, ...) {if (!(Expr)){B_LOG_BASE(Error, "[ASSERTION] EXPR : %s , DESC : %s", TEXT("'"#Expr"'"), *FString::Printf(TEXT(Format), ##__VA_ARGS__));}}
#define B_ASSERTR_DEV(Expr, Format, ...) {if (!(Expr)){B_LOG_BASE(Error, "[ASSERTION] EXPR : %s , DESC : %s", TEXT("'"#Expr"'"), *FString::Printf(TEXT(Format), ##__VA_ARGS__));return __VA_ARGS__;}}
#define B_COMPILE_DEV(Expr, Message)  typedef char msg[(Expr) ? 1 : -1];