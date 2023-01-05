SELECT DISTINCT pokemon.name
FROM evolution e1, evolution e2, pokemon
WHERE e1.after_id <> e2.before_id
AND e1.after_id NOT IN (
        SELECT e2.after_id
        FROM evolution e1, evolution e2
        WHERE e1.after_id = e2.before_id
        AND e2.after_id <> e1.before_id)
AND pokemon.id = e1.after_id
ORDER BY pokemon.name;