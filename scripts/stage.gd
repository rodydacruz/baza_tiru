extends Node2D

var is_game_over = false
var events = 0
var asteroid = preload("res://scenes/asteroid.tscn")
var dark = preload("res://scenes/dark.tscn")
var low_nemy = preload("res://scenes/low_nemy.tscn")
var boss_nemy01 = preload("res://scenes/boss_nemy01.tscn")


var score = 100

func _ready():
	get_node("player").connect("destroyed", Callable(self, "_on_player_destroyed"))
	get_node("spawn_timer").connect("timeout", Callable(self, "_on_spawn_timer_timeout"))
	
func _input(event):
	if Input.is_key_pressed(KEY_ESCAPE):
		get_tree().quit()
	if is_game_over and Input.is_key_pressed(KEY_ENTER):
		get_tree().change_scene_to_file("res://scenes/stage.tscn")
		
		
func _on_player_destroyed():
	get_node("ui/retry").show()
	is_game_over = true
	

func _on_spawn_timer_timeout():
	var asteroid_instance = asteroid.instantiate()
	asteroid_instance.position = Vector2(Settings.SCREEN_WIDTH + 8, randf_range(0,Settings.SCREEN_HEIGHT))
	asteroid_instance.connect("score", Callable(self, "_on_player_score"))
	add_child(asteroid_instance)
	
	if randi() % 5 == 0:
		var enemy_instance = low_nemy.instantiate()
		enemy_instance.position = Vector2(Settings.SCREEN_WIDTH + 16,randf_range(20,Settings.SCREEN_HEIGHT - 20))
		enemy_instance.connect("score", Callable(self, "_on_player_score"))
		
		add_child(enemy_instance)
	
	
func _on_player_score(points, enemy_id):
	score += points
	get_node("ui/score").text = "Score: " + str(score)
	print("Enemy defeated with ID:", enemy_id, " +", points, "points")
	_runScorePhase(score)

func getPlayerPosition():
	return get_node("player").position

func _runScorePhase(score):
	if events <= 0:  
		if score > 20 && score < 100:
			# Creating a Field Of Darks
			for d in [1,2,3,4,5,6,7,8,9,10,11,12]:
				var dark_instance = dark.instantiate()
				var position = randf_range(Settings.SCREEN_WIDTH + 8,2 * Settings.SCREEN_WIDTH)
				events += 1
				dark_instance.position = Vector2(position, randf_range(0,Settings.SCREEN_HEIGHT))
				dark_instance.connect("score", Callable(self, "_on_player_score"))
		
				add_child(dark_instance)
		if score> 100 && score <200:
			# Lunch BossNemy01
			events = +1
			var boss_nemy01_instance = boss_nemy01.instantiate()
			boss_nemy01_instance.position = Vector2(Settings.SCREEN_WIDTH - 20,randf_range(0,Settings.SCREEN_HEIGHT))
			boss_nemy01_instance.connect("score", Callable(self, "_on_player_score"))
			
			add_child(boss_nemy01_instance)
			pass
			
	print("Reched Score",score)
	
func executeCheatCode():
	pass
