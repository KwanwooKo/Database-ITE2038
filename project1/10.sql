SELECT pokemon.type
FROM (
    SELECT DISTINCT pokemon.id
    FROM evolution, pokemon
    WHERE pokemon.id = evolution.before_id OR pokemon.id = evolution.after_id
    ORDER BY pokemon.id) AS combinedEv(id), pokemon
WHERE combinedEv.id = pokemon.id
GROUP BY pokemon.type
HAVING COUNT(pokemon.type) >= 3
ORDER BY pokemon.type;