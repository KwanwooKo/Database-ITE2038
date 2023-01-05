SELECT DISTINCT pokemon.name
FROM pokemon
WHERE (pokemon.id NOT IN (
        SELECT before_id
        FROM evolution)
OR pokemon.id NOT IN (
        SELECT after_id
        FROM evolution))
AND pokemon.id NOT IN (
        SELECT before_id
        FROM evolution)
AND pokemon.type = 'Water'
ORDER BY pokemon.name;