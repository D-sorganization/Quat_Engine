import logging
from typing import Callable

logger = logging.getLogger(__name__)


def require(condition: Callable[..., bool], message: str = "Precondition failed"):
    def decorator(func):
        def wrapper(*args, **kwargs):
            if not condition(*args, **kwargs):
                logger.error(
                    "Precondition check failed",
                    extra={
                        "function": func.__name__,
                        "contract_message": message,
                        "arg_count": len(args),
                        "kwarg_count": len(kwargs),
                    },
                )
                raise ValueError(message)
            logger.debug(
                "Precondition check passed",
                extra={"function": func.__name__},
            )
            return func(*args, **kwargs)

        return wrapper

    return decorator


def ensure(condition: Callable[..., bool], message: str = "Postcondition failed"):
    def decorator(func):
        def wrapper(*args, **kwargs):
            result = func(*args, **kwargs)
            if not condition(result):
                logger.error(
                    "Postcondition check failed",
                    extra={
                        "function": func.__name__,
                        "contract_message": message,
                        "result_type": type(result).__name__,
                    },
                )
                raise RuntimeError(message)
            logger.debug(
                "Postcondition check passed",
                extra={"function": func.__name__},
            )
            return result

        return wrapper

    return decorator
