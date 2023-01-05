SELECT DISTINCT firstpokemon.id, firstpokemon.name, middlepokemon.name, finalpokemon.name
FROM pokemon firstpokemon, pokemon middlepokemon, pokemon finalpokemon, evolution e1, evolution e2
WHERE e1.before_id = firstpokemon.id
AND e1.after_id = middlepokemon.id
AND e1.after_id = e2.before_id
AND e2.after_id = finalpokemon.id
ORDER BY firstpokemon.id;