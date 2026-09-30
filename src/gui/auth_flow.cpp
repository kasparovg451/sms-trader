#include "auth_flow.h"

AuthFlowResult runAuthFlow(
    AuthIntent intent,
    const AuthOperation& registerOperation,
    const AuthOperation& loginOperation
) {
    if (intent == AuthIntent::Register) {
        const AuthFlowResult registration = registerOperation();
        if (!registration.success) {
            return registration;
        }
    }
    return loginOperation();
}
