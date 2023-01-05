SELECT DISTINCT pokemon.name
FROM pokemon
WHERE (pokemon.name LIKE 'A%'
        OR pokemon.name LIKE 'E%'
        OR pokemon.name LIKE 'I%'
        OR pokemon.name LIKE 'O%'
        OR pokemon.name LIKE 'U%')
ORDER BY pokemon.name;