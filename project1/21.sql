SELECT COUNT(pokemon.type), round(((COUNT(pokemoncount.num)::float / (SELECT COUNT(*) FROM pokemon)::float) * 100)::numeric, 2)
FROM pokemon, (
        SELECT pokemon.type, COUNT(pokemon.type)
        FROM pokemon
        GROUP BY pokemon.type) AS pokemoncount(type, num)
WHERE pokemon.type = pokemoncount.type
AND pokemoncount.num = (
        SELECT MAX(pokemoncount.num)
        FROM pokemon, (
            SELECT pokemon.type, COUNT(pokemon.type)
            FROM pokemon
            GROUP BY pokemon.type) AS pokemoncount(type, num)
        WHERE pokemoncount.type = pokemon.type);